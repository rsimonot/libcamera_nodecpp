#include "cam.hpp"
// Some includes are missing on purpose, I wanna get the error and be obligated to look at the functions implementation before adding the libs here

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
 * 
 * Flow :
 * 1/ Camera Manager
 * 2/ Acquire one particular camera
 * 3/ Generate and validate its config + Configure it
 * 4/ Allocate memory for the streaming (frame buffers)
 * 5/ Create requests for every frame buffer
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
	camera->acquire();

	// Generating camera configuration
	std::unique_ptr<libcamera::CameraConfiguration> config = camera->generateConfiguration( { libcamera::StreamRole::Viewfinder } );

	// Generating Steam configuration for the camera config
	// Once again the first element is a proper default one for us
	libcamera::StreamConfiguration &streamConfig = config->at(0);

	config->validate();		// adjunsting it so it's recognized
	camera->configure(config.get());

	// The images captured while streaming have to be stored in buffers
	// Using libcamera's FrameBufferAllocator which determines sizes and types on his own
	libcamera::FrameBufferAllocator *allocator = new libcamera::FrameBufferAllocator(camera);
	//  ~ Control ~
	if (allocator->allocate(streamConfig.stream()) < 0) {
		std::cerr << "Can't allocate buffers" << std::endl;
		return 2;
	}

	// Preparing the stream, using requests on frame buffers to contain the captured images
	libcamera::Stream *stream = streamConfig.stream();

	const std::vector<std::unique_ptr<libcamera::FrameBuffer>> &buffers = allocator->buffers(stream);
	std::vector<std::unique_ptr<libcamera::Request>> requests;
	// Creating a request for each frame buffer, that'll be queued to the camera, which will then fill it with images
	for (unsigned int i = 0; i < buffers.size(); ++i) {
		std::unique_ptr<libcamera::Request> request = camera->createRequest();	// Initialize a request
		if (!request)
		{
			std::cerr << "Can't create request" << std::endl;
			return 3;
		}

		const std::unique_ptr<libcamera::FrameBuffer> &buffer = buffers[i];
		int ret = request->addBuffer(stream, buffer.get());	// Adding current buffer to a request
		if (ret < 0)
		{
			std::cerr << "Can't set buffer for request"
				  << std::endl;
			return 4;
		}

		// No additionnal controls on the request for now (ex. Brightness...)

		requests.push_back(std::move(request));
	}	// After this loop we got as many <request> objets in "requests" as the number of buffers created by the FrameBufferAllocator

	// The camera needs now to fill our buffers

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