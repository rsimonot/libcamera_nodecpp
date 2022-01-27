#include "cam.hpp"

static std::shared_ptr<libcamera::Camera> camera;

/**
 * @brief Retrieves the camera informations - To be called by other functions of the namespace
 * 
 * @param camera => A camera acquired through a CameraManager
 * @return std::string => The infos about the camera
 */
std::string cam::getCameraInfos(libcamera::Camera *camera)
{
    const libcamera::ControlList &cam_properties = camera->properties();
	std::string name;

	switch (cam_properties.get(libcamera::properties::Location)) {
	case libcamera::properties::CameraLocationFront:
		name = "Internal front camera";
		break;
	case libcamera::properties::CameraLocationBack:
		name = "Internal back camera";
		break;
	case libcamera::properties::CameraLocationExternal:
		name = "External camera";
		if (cam_properties.contains(libcamera::properties::Model))
			name += " '" + cam_properties.get(libcamera::properties::Model) + "'";
		break;
	}

	name += " (" + camera->id() + ")";

	return name;
}

/**
 * @brief Steams a camera. A default camera will automatically be used, as there's only one for now on the system
 * 
 * @return int => Classic value, 0 means OK
 */
int cam::cameraStream()
{
	// Creating a camera manager, that will be able to access cameras
	std::unique_ptr<libcamera::CameraManager> cam_mng = std::make_unique<libcamera::CameraManager>();
	cam_mng->start();

	// Iterating through cameras to get the infos, but we normally have only one in the DISO
	for (auto const &camera : cam_mng->cameras())
		std::cout << " - " << getCameraInfos(camera.get()) << std::endl;

	if (cam_mng->cameras().empty()) {
		std::cout << "No camera identified on the system." << std::endl;
		cam_mng->stop();
		return 1;
	}

	// Selecting camera #0 as default camera
	std::string cameraId = cam_mng->cameras()[0]->id();
	camera = cam_mng->get(cameraId);
	// Hands on the camera :
	camera->acquire();

	return 0;
}

/**
 * @brief Wrapping Stream function for Node-API
 * 
 * @return Napi::Number => Classic value, 0 means OK
 */
Napi::Number cam::cameraStreamWrapped(const Napi::CallbackInfo& info)
{
	Napi::Env env = info.Env();
	Napi::Number res = Napi::Number::New(env, cam::cameraStream());

	return res;
}


Napi::Object cam::Init(Napi::Env env, Napi::Object exports)
{
	exports.Set(
		"getCamStream",
		Napi::Function::New(env, cam::cameraStreamWrapped)
	);

	return exports;
}