/**
 * @file tesseract_init_info.h
 * @brief This is a container used to store information about how a given Tesseract was initialized.
 *
 * @author Matthew Powelson
 * @date March 17, 2020
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
#ifndef TESSERACT_TESSERACT_INIT_INFO_H
#define TESSERACT_TESSERACT_INIT_INFO_H

#include <tesseract_common/macros.h>
TESSERACT_COMMON_IGNORE_WARNINGS_PUSH
#include <boost/filesystem.hpp>
TESSERACT_COMMON_IGNORE_WARNINGS_POP

#include <tesseract/manipulator_manager.h>
#include <tesseract_environment/core/environment.h>
#include <tesseract_scene_graph/resource_locator.h>
#include <tesseract_scene_graph/graph.h>
#include <tesseract_scene_graph/srdf_model.h>

namespace tesseract
{
/** @brief Used to specify which init method was used to construct a Tesseract*/
enum class TesseractInitType
{
  SCENE_GRAPH,
  SCENE_GRAPH_SRDF_MODEL,
  URDF_STRING,
  URDF_STRING_SRDF_STRING,
  URDF_PATH,
  URDF_PATH_SRDF_PATH,
  ENVIRONMENT_MANIPULATOR_MANAGER
};

/** @brief Used to store information about how a given Tesseract was initialized. See the Tesseract init methods.

Note: Recreating a Tesseract does not guarantee that it is identical to the Tesseract associate with this construction
info since the kinmeatics managers could have changed and it does not include the environment command history. This will
simply recreate the Tesseract as it was at construction.  */
struct TesseractInitInfo
{
  using Ptr = std::shared_ptr<TesseractInitInfo>;
  using ConstPtr = std::shared_ptr<const TesseractInitInfo>;

  /** @brief Specifies which members should be used to recreate the Tesseract*/
  TesseractInitType type;

  /**
   * @brief Used when InitType is SCENE_GRAPH or SCENE_GRAPH_SRDF_MODEL
   *
   * This stores a clone of the scene graph
   */
  tesseract_scene_graph::SceneGraph::Ptr scene_graph;

  /**
   * @brief Used when InitType is SCENE_GRAPH_SRDF_MODEL
   *
   * This stores a clone of the srdf model
   */
  tesseract_scene_graph::SRDFModel::Ptr srdf_model;

  /**
   * @brief Used when InitType is ENVIRONMENT_MANIPULATOR_MANAGER
   *
   * This store a clone of the environment
   */
  tesseract_environment::Environment::Ptr environment;

  /**
   * @brief Used when InitType is ENVIRONMENT_MANIPULATOR_MANAGER
   *
   * This stores a clone of the manipulator manager
   */
  tesseract::ManipulatorManager::Ptr manipulator_manager;

  /** @brief Used when InitType is URDF_STRING and URDF_STRING_SRDF_STRING*/
  std::string urdf_string;
  /** @brief Used when InitType is URDF_STRING_SRDF_STRING*/
  std::string srdf_string;

  /** @brief Used when InitType is URDF_PATH and URDF_PATH_SRDF_PATH*/
  boost::filesystem::path urdf_path;
  /** @brief Used when InitType is URDF_PATH_SRDF_PATH*/
  boost::filesystem::path srdf_path;

  /** @brief Used when InitType is URDF_STRING, URDF_STRING_SRDF_STRING, URDF_PATH, and URDF_PATH_SRDF_PATH */
  tesseract_scene_graph::ResourceLocator::Ptr resource_locator;
};

}  // namespace tesseract
#endif  // TESSERACT_TESSERACT_H