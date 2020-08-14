/**
 * @file visualization_loader.h
 * @brief Visualization Loader
 *
 * @author Levi Armstrong
 * @date July 29, 2020
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

#include <tesseract_common/macros.h>
TESSERACT_COMMON_IGNORE_WARNINGS_PUSH
#include <console_bridge/console.h>
#include <cstdlib>
TESSERACT_COMMON_IGNORE_WARNINGS_POP

#include <tesseract_visualization/visualization_loader.h>

const std::string TESSERACT_IGNITION_LIB_DIR = "/snap/tesseract-ignition/current/opt/ros/melodic/lib";

const std::string TESSERACT_IGNITION_LIBRARY = TESSERACT_IGNITION_LIB_DIR + "/libtesseract_ignition_visualization_"
                                                                            "plugin.so";
const std::string TESSERACT_IGNITION_CLASS = "tesseract_ignition::TesseractIgnitionVisualization";

const std::string TESSERACT_VISUALIZATION_LIBRARY_ENV = "TESSERACT_VISUALIZATION_PLUGIN_LIBRARY";
const std::string TESSERACT_VISUALIZATION_CLASS_ENV = "TESSERACT_VISUALIZATION_PLUGIN_CLASS";

namespace tesseract_visualization
{
VisualizationLoader::VisualizationLoader()
  : library_path_(TESSERACT_IGNITION_LIBRARY), derived_class_(TESSERACT_IGNITION_CLASS)
{
  // Check for environment variable to override default library
  const char* env_library = std::getenv(TESSERACT_VISUALIZATION_LIBRARY_ENV.c_str());
  const char* env_class = std::getenv(TESSERACT_VISUALIZATION_CLASS_ENV.c_str());
  if (env_library && env_class)
  {
    library_path_ = std::string(env_library);
    derived_class_ = std::string(env_class);
    createLoader(library_path_);
  }
  else if (env_library)
  {
    library_path_ = std::string(env_library);
    createLoader(library_path_);
  }
  else
  {
    const char* env_ld_library_path = std::getenv("LD_LIBRARY_PATH");
    if (env_ld_library_path)
      setenv("LD_LIBRARY_PATH", (std::string(env_ld_library_path) + ":" + TESSERACT_IGNITION_LIB_DIR).c_str(), true);
    else
      setenv("LD_LIBRARY_PATH", TESSERACT_IGNITION_LIB_DIR.c_str(), true);

    createLoader(library_path_);
  }
}

VisualizationLoader::VisualizationLoader(const std::string& library_path, const std::string& derived_class)
  : library_path_(library_path), derived_class_(derived_class)
{
  createLoader(library_path_);
}

Visualization::Ptr VisualizationLoader::get()
{
  // Dynamically load visualization library
  if (loader_)
  {
    try
    {
      if (loader_->isClassAvailable<tesseract_visualization::Visualization>(derived_class_))
      {
#ifndef CLASS_LOADER_LESS_0_4_0
        return loader_->createSharedInstance<tesseract_visualization::Visualization>(derived_class_);
#else
        CONSOLE_BRIDGE_logWarn("Only supported with class loader version 4.0 and greater");
        return nullptr;
#endif
      }
      else
      {
      }
    }
    catch (const std::exception&)
    {
      CONSOLE_BRIDGE_logWarn(
          "Failed to load class '%s' from library '%s'", library_path_.c_str(), derived_class_.c_str());
      return nullptr;
    }
  }

  return nullptr;
}

void VisualizationLoader::createLoader(const std::string& library_path)
{
  try
  {
    loader_ = std::make_shared<class_loader::ClassLoader>(library_path);
  }
  catch (const std::exception&)
  {
    loader_ = nullptr;
    CONSOLE_BRIDGE_logWarn("Failed to load library '%s'", library_path_.c_str());
  }
}

}  // namespace tesseract_visualization