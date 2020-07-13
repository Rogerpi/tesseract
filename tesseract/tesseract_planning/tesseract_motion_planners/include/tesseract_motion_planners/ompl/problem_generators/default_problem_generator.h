/**
 * @file default_problem_generator.h
 * @brief Generates a OMPL problem from a planner request
 *
 * @author Levi Armstrong
 * @date April 18, 2018
 * @version TODO
 * @bug No known bugs
 *
 * @copyright Copyright (c) 2020, Southwest Research Institute
 *
 * @par License
 * Software License Agreement (Apache License)
 * @par
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * http://www.apache.org/licenses/LICENSE-2.0
 * @par
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef TESSERACT_MOTION_PLANNERS_OMPL_DEFAULT_PROBLEM_GENERATOR_H
#define TESSERACT_MOTION_PLANNERS_OMPL_DEFAULT_PROBLEM_GENERATOR_H

#include <tesseract_motion_planners/ompl/profile/ompl_default_plan_profile.h>
#include <tesseract_kinematics/core/validate.h>
#include <tesseract/tesseract.h>
#include <tesseract_motion_planners/core/types.h>
#include <vector>

namespace tesseract_planning
{
inline OMPLProblem::UPtr CreateOMPLSubProblem(const PlannerRequest& request,
                                              const tesseract_kinematics::ForwardKinematics::Ptr& manip_fwd_kin,
                                              const tesseract_kinematics::InverseKinematics::Ptr& manip_inv_kin,
                                              const std::vector<std::string>& active_link_names)
{
  auto sub_prob = std::make_unique<OMPLProblem>();
  sub_prob->tesseract = request.tesseract;
  sub_prob->env_state = request.env_state;
  sub_prob->state_solver = request.tesseract->getEnvironmentConst()->getStateSolver();
  sub_prob->state_solver->setState(request.env_state->joints);
  sub_prob->manip_fwd_kin = manip_fwd_kin;
  sub_prob->manip_inv_kin = manip_inv_kin;
  sub_prob->contact_checker = request.tesseract->getEnvironmentConst()->getDiscreteContactManager();
  sub_prob->contact_checker->setCollisionObjectsTransform(request.env_state->link_transforms);
  sub_prob->contact_checker->setActiveCollisionObjects(active_link_names);
  return sub_prob;
}

inline std::vector<OMPLProblem::UPtr> DefaultOMPLProblemGenerator(const PlannerRequest& request,
                                                                  const OMPLPlanProfileMap& plan_profiles)
{
  std::vector<OMPLProblem::UPtr> problem;
  std::vector<std::string> active_link_names_;
  tesseract_kinematics::ForwardKinematics::Ptr manip_fwd_kin_;
  tesseract_kinematics::InverseKinematics::Ptr manip_inv_kin_;

  manip_fwd_kin_ = request.tesseract->getFwdKinematicsManagerConst()->getFwdKinematicSolver(request.manipulator);
  if (request.manipulator_ik_solver.empty())
    manip_inv_kin_ = request.tesseract->getInvKinematicsManagerConst()->getInvKinematicSolver(request.manipulator);
  else
    manip_inv_kin_ = request.tesseract->getInvKinematicsManagerConst()->getInvKinematicSolver(
        request.manipulator, request.manipulator_ik_solver);
  if (!manip_fwd_kin_)
  {
    CONSOLE_BRIDGE_logError("No Forward Kinematics solver found");
    return problem;
  }
  if (!manip_inv_kin_)
  {
    CONSOLE_BRIDGE_logError("No Inverse Kinematics solver found");
    return problem;
  }
  // Process instructions
  if (!tesseract_kinematics::checkKinematics(manip_fwd_kin_, manip_inv_kin_))
    CONSOLE_BRIDGE_logError("Check Kinematics failed. This means that Inverse Kinematics does not agree with KDL "
                            "(TrajOpt). Did you change the URDF recently?");

  // Get Active Link Names
  {
    std::vector<std::string> active_link_names = manip_inv_kin_->getActiveLinkNames();
    auto adjacency_map =
        std::make_shared<tesseract_environment::AdjacencyMap>(request.tesseract->getEnvironmentConst()->getSceneGraph(),
                                                              active_link_names,
                                                              request.env_state->link_transforms);
    active_link_names_ = adjacency_map->getActiveLinkNames();
  }

  // Check and make sure it does not contain any composite instruction
  for (const auto& instruction : request.instructions)
    if (instruction.isComposite())
      throw std::runtime_error("OMPL planner does not support child composite instructions.");

  Waypoint start_waypoint = NullWaypoint();
  if (request.instructions.hasStartWaypoint())
  {
    start_waypoint = request.instructions.getStartWaypoint();
  }
  else
  {
    Eigen::VectorXd current_jv = request.env_state->getJointValues(manip_fwd_kin_->getJointNames());
    JointWaypoint temp(current_jv);
    temp.joint_names = manip_fwd_kin_->getJointNames();
    start_waypoint = temp;
  }

  // Transform plan instructions into ompl problem
  int index = 0;
  for (std::size_t i = 0; i < request.instructions.size(); ++i)
  {
    const auto& instruction = request.instructions[i];
    if (instruction.isPlan())
    {
      // Save plan index for process trajectory
//      plan_instruction_indices_.push_back(i);

      assert(instruction.getType() == static_cast<int>(InstructionType::PLAN_INSTRUCTION));
      const auto* plan_instruction = instruction.cast_const<PlanInstruction>();

      assert(request.seed[i].isComposite());
      const auto* seed_composite = request.seed[i].cast_const<tesseract_planning::CompositeInstruction>();

      // Get Plan Profile
      std::string profile = plan_instruction->getProfile();
      if (profile.empty())
        profile = "DEFAULT";

      typename OMPLPlanProfile::Ptr cur_plan_profile{ nullptr };
      auto it = plan_profiles.find(profile);
      if (it == plan_profiles.end())
        cur_plan_profile = std::make_shared<OMPLDefaultPlanProfile>();
      else
        cur_plan_profile = it->second;

      /** @todo Should check that the joint names match the order of the manipulator */
      OMPLProblem::UPtr sub_prob = CreateOMPLSubProblem(request, manip_fwd_kin_, manip_inv_kin_, active_link_names_);
      cur_plan_profile->setup(*sub_prob);
      sub_prob->n_output_states = static_cast<int>(seed_composite->size());

      if (plan_instruction->isLinear())
      {
        /** @todo Add support for linear motion to ompl planner */
        if (isCartesianWaypoint(plan_instruction->getWaypoint().getType()))
        {
          // TODO Currently skipping linear moves until SE3 motion planning is implemented.
          problem.push_back(nullptr);
          ++index;
        }
        else if (isJointWaypoint(plan_instruction->getWaypoint().getType()))
        {
          // TODO Currently skipping linear moves until SE3 motion planning is implemented.
          problem.push_back(nullptr);
          ++index;
        }
        else
        {
          throw std::runtime_error("OMPLMotionPlannerDefaultConfig: unknown waypoint type");
        }
      }
      else if (plan_instruction->isFreespace())
      {
        if (isJointWaypoint(plan_instruction->getWaypoint().getType()))
        {
          const auto* cur_wp = plan_instruction->getWaypoint().cast_const<tesseract_planning::JointWaypoint>();
          cur_plan_profile->applyGoalStates(*sub_prob, *cur_wp, *plan_instruction, active_link_names_, index);

          if (index == 0)
          {
            ompl::base::ScopedState<> start_state(sub_prob->simple_setup->getStateSpace());
            if (isJointWaypoint(start_waypoint.getType()))
            {
              const auto* prev_wp = start_waypoint.cast_const<tesseract_planning::JointWaypoint>();
              cur_plan_profile->applyStartStates(*sub_prob, *prev_wp, *plan_instruction, active_link_names_, index);
            }
            else if (isCartesianWaypoint(start_waypoint.getType()))
            {
              const auto* prev_wp = start_waypoint.cast_const<tesseract_planning::CartesianWaypoint>();
              cur_plan_profile->applyStartStates(*sub_prob, *prev_wp, *plan_instruction, active_link_names_, index);
            }
            else
            {
              throw std::runtime_error("OMPLMotionPlannerDefaultConfig: unknown waypoint type");
            }

            problem.push_back(std::move(sub_prob));
            ++index;
          }
          else
          {
            /** @todo Update. Extract the solution for the previous plan and set as the start */
            assert(false);
          }
        }
        else if (isCartesianWaypoint(plan_instruction->getWaypoint().getType()))
        {
          const auto* cur_wp = plan_instruction->getWaypoint().cast_const<tesseract_planning::CartesianWaypoint>();
          cur_plan_profile->applyGoalStates(*sub_prob, *cur_wp, *plan_instruction, active_link_names_, index);

          if (index == 0)
          {
            ompl::base::ScopedState<> start_state(sub_prob->simple_setup->getStateSpace());
            if (isJointWaypoint(start_waypoint.getType()))
            {
              const auto* prev_wp = start_waypoint.cast_const<tesseract_planning::JointWaypoint>();
              cur_plan_profile->applyStartStates(*sub_prob, *prev_wp, *plan_instruction, active_link_names_, index);
            }
            else if (isCartesianWaypoint(start_waypoint.getType()))
            {
              const auto* prev_wp = start_waypoint.cast_const<tesseract_planning::CartesianWaypoint>();
              cur_plan_profile->applyStartStates(*sub_prob, *prev_wp, *plan_instruction, active_link_names_, index);
            }
            else
            {
              throw std::runtime_error("OMPLMotionPlannerDefaultConfig: unknown waypoint type");
            }
          }
          else
          {
            /** @todo Update. Extract the solution for the previous plan and set as the start */
            assert(false);
          }

          problem.push_back(std::move(sub_prob));
          ++index;
        }
      }
      else
      {
        throw std::runtime_error("OMPLMotionPlannerDefaultConfig: Unsupported!");
      }

      start_waypoint = plan_instruction->getWaypoint(); /** @todo need to extract the solution */
    }
  }

  return problem;
}
}  // namespace tesseract_planning
#endif