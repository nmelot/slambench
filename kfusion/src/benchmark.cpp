/*

 Copyright (c) 2014 University of Edinburgh, Imperial College, University of Manchester.
 Developed in the PAMELA project, EPSRC Programme Grant EP/K008730/1

 This code is licensed under the MIT License.

 */

#include <kernels.h>
#include <interface.h>
#include <stdint.h>
#include <vector>
#include <sstream>
#include <string>
#include <cstring>
#include <time.h>
#include <csignal>

#include <sys/types.h>
#include <sys/stat.h>
#include <sstream>
#include <iomanip>
#include <getopt.h>

#include <drake.h>
#include <drake/intel-ia.h>
#include <drake-kfusion.h>

#ifndef DEBUG
#define DEBUG 1
#endif

#if DEBUG != 0
#define debug(var) std::cout << "[" << __FILE__ << ":" << __FUNCTION__ << ":" << __LINE__ << "] " << #var << " = \"" << var << "\"" << std::endl;
#else
#define debug(var)
#endif

drake_declare(drake_kfusion)

inline double tock() {
	synchroniseDevices();
#ifdef __APPLE__
		clock_serv_t cclock;
		mach_timespec_t clockData;
		host_get_clock_service(mach_host_self(), SYSTEM_CLOCK, &cclock);
		clock_get_time(cclock, &clockData);
		mach_port_deallocate(mach_task_self(), cclock);
#else
		struct timespec clockData;
		clock_gettime(CLOCK_MONOTONIC, &clockData);
#endif
		return (double) clockData.tv_sec + clockData.tv_nsec / 1000000000.0;
}	



/***
 * This program loop over a scene recording
 */

void matrix4ToFloat16(float out[16], const Matrix4 &in)
{
	out[0] = in.data[0].x;
	out[1] = in.data[0].y;
	out[2] = in.data[0].z;
	out[3] = in.data[0].w;
	out[4] = in.data[1].x;
	out[5] = in.data[1].y;
	out[6] = in.data[1].z;
	out[7] = in.data[1].w;
	out[8] = in.data[2].x;
	out[9] = in.data[2].y;
	out[10] = in.data[2].z;
	out[11] = in.data[2].w;
	out[12] = in.data[3].x;
	out[13] = in.data[3].y;
	out[14] = in.data[3].z;
	out[15] = in.data[3].w;
}

int main(int argc, char ** argv) {

	Configuration config(argc, argv);

	// ========= CHECK ARGS =====================

	std::ostream* logstream = &std::cout;
	std::ofstream logfilestream;
	assert(config.compute_size_ratio > 0);
	assert(config.integration_rate > 0);
	assert(config.volume_size.x > 0);
	assert(config.volume_resolution.x > 0);

	if (config.log_file != "") {
		logfilestream.open(config.log_file.c_str());
		logstream = &logfilestream;
	}
	if (config.input_file == "") {
		std::cerr << "No input found." << std::endl;
		config.print_arguments();
		exit(1);
	}

	// ========= READER INITIALIZATION  =========

	DepthReader * reader;

	if (is_file(config.input_file)) {
		reader = new RawDepthReader(config.input_file, config.fps,
				config.blocking_read);

	} else {
		reader = new SceneDepthReader(config.input_file, config.fps,
				config.blocking_read);
	}

	std::cout.precision(10);
	std::cerr.precision(10);

	float3 init_pose = config.initial_pos_factor * config.volume_size;
	const uint2 inputSize = reader->getinputSize();
	std::cerr << "input Size is = " << inputSize.x << "," << inputSize.y
			<< std::endl;

	//  =========  BASIC PARAMETERS  (input size / computation size )  =========

	const uint2 computationSize = make_uint2(
			inputSize.x / config.compute_size_ratio,
			inputSize.y / config.compute_size_ratio);
	float4 camera = reader->getK() / config.compute_size_ratio;

	if (config.camera_overrided)
		camera = config.camera / config.compute_size_ratio;
	//  =========  BASIC BUFFERS  (input / output )  =========

	// Construction Scene reader and input buffer
	uint16_t* inputDepth = (uint16_t*) malloc(
			sizeof(uint16_t) * inputSize.x * inputSize.y);
	uchar4* depthRender = (uchar4*) malloc(
			sizeof(uchar4) * computationSize.x * computationSize.y);
	uchar4* trackRender = (uchar4*) malloc(
			sizeof(uchar4) * computationSize.x * computationSize.y);
	uchar4* volumeRender = (uchar4*) malloc(
			sizeof(uchar4) * computationSize.x * computationSize.y);

	uint frame = 0;

	Kfusion kfusion(computationSize, config.volume_resolution,
			config.volume_size, init_pose, config.pyramid);

	double timings[7];
	timings[0] = tock();

	*logstream
			<< "frame\tacquisition\tpreprocessing\ttracking\tintegration\traycasting\trendering\tcomputation\ttotal    \tX          \tY          \tZ         \ttracked   \tintegrated"
			<< std::endl;
	logstream->setf(std::ios::fixed, std::ios::floatfield);

	ia_arguments_t args;
	args.num_cores = 1; 
	args.poll_at_idle = 1;
	args.wait_after_scaling = 0;
	drake_platform_t stream;
	drake_kfusion_t drake_init;
	stream = drake_platform_init(&args);
	drake_platform_stream_create(stream, drake_kfusion);

	// Initialize stream (The scc requires this phase to not be run in parallel because of extensive IOs when loading input)

	while (reader->readNextDepthFrame(inputDepth)) {
	drake_init.in = inputDepth;
	drake_init.inSize_x = inputSize.x;
	drake_init.inSize_y = inputSize.y;
	drake_init.compSize_x = computationSize.x;
	drake_init.compSize_y = computationSize.y;
	drake_init.gaussian = gaussian; 
	drake_init.e_d = e_delta;
	drake_init.r = radius;
	drake_init.dist_threshold = 0.1f;
	drake_init.normal_threshold = 0.8f;
	matrix4ToFloat16(drake_init.pose, kfusion.getPose());
	drake_init.iteration = (unsigned int*)malloc(sizeof(int) * drake_init.iteration_size);
	size_t buffer_size = 0;
	drake_init.iteration_size = config.pyramid.size();
	//drake_init.iteration_size = 1; // TODO: remove
	for(std::vector<int>::iterator i = config.pyramid.begin(); i != config.pyramid.end(); i++)
	{
		size_t ii = std::distance(config.pyramid.begin(), i);
		debug(ii);
		debug(*i);
		drake_init.iteration[ii] = *i;
		//drake_init.iteration[ii] = 1; // TODO: remove
		debug(drake_init.iteration[ii]);
		buffer_size += *i;
	}
	drake_init.pose[0] = init_pose.x;
	drake_init.pose[1] = init_pose.y;
	drake_init.pose[2] = init_pose.z;
	drake_init.vertex = (float*)calloc(computationSize.x * computationSize.y, sizeof(float));
	drake_init.normal = (float*)calloc(computationSize.x * computationSize.y, sizeof(float));
	drake_init.track_data = (float*)calloc(computationSize.x * computationSize.y, sizeof(TrackData));
	drake_init.out = (float*)malloc(sizeof(float) * (computationSize.x * computationSize.y) * ((pow(4, drake_init.iteration_size) - 1) / drake_init.iteration_size - 1) * 8 * buffer_size);
	memcpy(&drake_init.camera, &camera, sizeof(float) * 4);
	
	drake_platform_stream_init(stream, &drake_init);
	drake_platform_stream_run(stream);

		Matrix4 pose = kfusion.getPose();

		float xt = pose.data[0].w - init_pose.x;
		float yt = pose.data[1].w - init_pose.y;
		float zt = pose.data[2].w - init_pose.z;

		timings[1] = tock();

		kfusion.preprocessing(inputDepth, inputSize);

		timings[2] = tock();

		bool tracked = kfusion.tracking(camera, config.icp_threshold,
				config.tracking_rate, frame);

/*
		timings[3] = tock();

		bool integrated = kfusion.integration(camera, config.integration_rate,
				config.mu, frame);

		timings[4] = tock();

		bool raycast = kfusion.raycasting(camera, config.mu, frame);

		timings[5] = tock();

		kfusion.renderDepth(depthRender, computationSize);
		kfusion.renderTrack(trackRender, computationSize);
		kfusion.renderVolume(volumeRender, computationSize, frame,
				config.rendering_rate, camera, 0.75 * config.mu);

		timings[6] = tock();

		*logstream << frame << "\t" << timings[1] - timings[0] << "\t" //  acquisition
				<< timings[2] - timings[1] << "\t"     //  preprocessing
				<< timings[3] - timings[2] << "\t"     //  tracking
				<< timings[4] - timings[3] << "\t"     //  integration
				<< timings[5] - timings[4] << "\t"     //  raycasting
				<< timings[6] - timings[5] << "\t"     //  rendering
				<< timings[5] - timings[1] << "\t"     //  computation
				<< timings[6] - timings[0] << "\t"     //  total
				<< xt << "\t" << yt << "\t" << zt << "\t"     //  X,Y,Z
				<< tracked << "        \t" << integrated // tracked and integrated flags
				<< std::endl;
*/

		frame++;

		timings[0] = tock();

	size_t skip = 0;
	for(size_t i = 0; i < drake_init.iteration_size; i++)
	{
		//size_t i_skip = i == 0 ? 0 : (drake_init.compSize_x * drake_init.compSize_y) * (pow(4, i) - 1) / (pow(4, i) - pow(4, i - 1));
		for(size_t j = 0; j < drake_init.iteration[i]; j++)
		{
			size_t slambench_skip = 0;
			size_t size_x = drake_init.compSize_x / pow(2, i);
			for (size_t y = 0; y < drake_init.compSize_y / pow(2, i); y++)
			{
				for (size_t x = 0; x < size_x; x++)
				{
					if(((x == 0 && y == 0 && i == drake_init.iteration_size - 1 && j == drake_init.iteration[i] - 1) || 0) && 1)
					{
						debug(skip);
						if(
							trackingResult[slambench_skip].result != (int)drake_init.out[8 * (skip)] ||
							trackingResult[slambench_skip].error != drake_init.out[8 * (skip) + 1] ||
							trackingResult[slambench_skip].J[0] != drake_init.out[8 * (skip) + 2] ||
							trackingResult[slambench_skip].J[1] != drake_init.out[8 * (skip) + 3] ||
							trackingResult[slambench_skip].J[2] != drake_init.out[8 * (skip) + 4] ||
							trackingResult[slambench_skip].J[3] != drake_init.out[8 * (skip) + 5] ||
							trackingResult[slambench_skip].J[4] != drake_init.out[8 * (skip) + 6] ||
							trackingResult[slambench_skip].J[5] != drake_init.out[8 * (skip) + 7]
						)
						{
							printf("[%s:%s:%d] Output difference at index (%zu, %zu) (%zu) for i = %zu: got (%i, %.10f, %.10f, %.10f, %.10f, %.10f, %.10f, %.10f), expected (%d, %.10f, %.10f, %.10f, %.10f, %.10f, %.10f, %.10f)\n", __FILE__, __FUNCTION__, __LINE__, x, y, slambench_skip, i,
	//drake_init.out[skip], ScaledDepth[i][slambench_skip], log10(fabsf(drake_init.out[skip] - ScaledDepth[i][slambench_skip]))
								(int)drake_init.out[8 * (skip)],
								drake_init.out[8 * (skip) + 1],
								drake_init.out[8 * (skip) + 2],
								drake_init.out[8 * (skip) + 3],
								drake_init.out[8 * (skip) + 4],
								drake_init.out[8 * (skip) + 5],
								drake_init.out[8 * (skip) + 6],
								drake_init.out[8 * (skip) + 7],
								trackingResult[slambench_skip].result,
								trackingResult[slambench_skip].error,
								trackingResult[slambench_skip].J[0],
								trackingResult[slambench_skip].J[1],
								trackingResult[slambench_skip].J[2],
								trackingResult[slambench_skip].J[3],
								trackingResult[slambench_skip].J[4],
								trackingResult[slambench_skip].J[5]
							);
							abort();
						}
					}

					skip++;
					slambench_skip++;
				}
			}
		}
	}
	debug(skip);

	// TODO: get rid of these dynamic, repetitive allocation and free
	free(drake_init.out);
	free(drake_init.iteration);
	}
	drake_platform_stream_destroy(stream);
	drake_platform_destroy(stream);
	// ==========     DUMP VOLUME      =========

	if (config.dump_volume_file != "") {
	  kfusion.dumpVolume(config.dump_volume_file.c_str());
	}

	//  =========  FREE BASIC BUFFERS  =========

	free(inputDepth);
	free(depthRender);
	free(trackRender);
	free(volumeRender);
	return 0;

}

