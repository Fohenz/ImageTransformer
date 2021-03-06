/*
 * Copyright (c) 2016 Samsung Electronics Co., Ltd
 *
 * Licensed under the Flora License, Version 1.1 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://floralicense.org/license/
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "main.h"
#include "data.h"
#include <image_util.h>
#include <storage.h>
#include <dirent.h>
#include <sys/stat.h>

#define BUFLEN 256

static Evas_Object *image;
static transformation_h handle = NULL;
static media_packet_h media_packet = NULL;
static char *images_directory = NULL;
static char image_util_filename_encoded[BUFLEN];
static const char *resource_path;
static const char img_res_path[BUFLEN];
static bool transform_finished = false;
static media_packet_h packet_h;

extern struct view_info s_info;

/**
 * @brief Destroys the transformation handle and enables the buttons.
 * @details Called once for each supported JPEG encode/decode color space.
 * @remarks This function matches the Ecore_Task_Cb()
 *          type signature defined in the EFL API.
 *
 * @param data The user data passed via void pointer (not used here)
 *
 * @return @c EINA_TRUE on success
 */
Eina_Bool _btn_enable(void *data) {
	if (transform_finished) {

		image_util_transform_destroy(handle);
		handle = NULL;

		media_packet_destroy(packet_h);
		packet_h = NULL;

		for (app_button i = 0; i < BUTTON_COUNT; ++i)
			_disable_button(i, EINA_FALSE);

		transform_finished = false;
	}
	return EINA_TRUE;
}

/**
 * @brief Stores the image after the transformation.
 * @details Called when the transformation of the image is finished.
 *
 * @param dst The result buffer of image util transform
 * @param error_code The error code of image util transform
 * @param user_data The user data passed from the callback registration function
 */
static void _image_util_completed_cb(media_packet_h *dst, int error_code,
		void *user_data) {
	packet_h = *dst;
	PRINT_MSG("Transformation finished!");
	//dlog_print(DLOG_DEBUG, LOG_TAG, "Transformation finished.");
	if (error_code != IMAGE_UTIL_ERROR_NONE || dst == NULL) {
		dlog_print(DLOG_ERROR, LOG_TAG,
				"An error occurred during transformation.<br>Error code: %d.",
				error_code);
		PRINT_MSG("An error occurred during transformation.");
	} else {
		/* Get the transformed image format. */
		media_format_h fmt = NULL;

		int error_code = media_packet_get_format(*dst, &fmt);
		if (error_code != MEDIA_PACKET_ERROR_NONE) {
			DLOG_PRINT_ERROR("media_packet_get_format", error_code);
			PRINT_MSG("error media_packet_get_format");
			image_util_transform_destroy(handle);
			return;
		}

		/* Get the transformed image dimensions and MIME type. */
		media_format_mimetype_e mimetype;
		int width, height;

		error_code = media_format_get_video_info(fmt, &mimetype, &width,
				&height, NULL, NULL);
		if (error_code != MEDIA_FORMAT_ERROR_NONE) {
			DLOG_PRINT_ERROR("media_format_get_video_info", error_code);
			media_format_unref(fmt);
			image_util_transform_destroy(handle);
			return;
		}
		/* Release the memory allocated for the media format. */
		media_format_unref(fmt);

		/* Get the buffer where the transformed image is stored. */
		void *packet_buffer = NULL;

		error_code = media_packet_get_buffer_data_ptr(*dst, &packet_buffer);
		if (error_code != MEDIA_PACKET_ERROR_NONE) {
			DLOG_PRINT_ERROR("media_packet_get_buffer_data_ptr", error_code);
			PRINT_MSG("error media_packet_get_buffer_data_ptr");
			image_util_transform_destroy(handle);
			return;
		}

		if (mimetype == MEDIA_FORMAT_NV12) {
			/* Store the image from the buffer in a file. */
			error_code = image_util_encode_jpeg(packet_buffer, width, height,
					IMAGE_UTIL_COLORSPACE_NV12, 100,
					image_util_filename_encoded);
			if (error_code != IMAGE_UTIL_ERROR_NONE) {
				DLOG_PRINT_ERROR("image_util_encode_jpeg", error_code);
				PRINT_MSG("error image_util_encode_jpeg");
				image_util_transform_destroy(handle);
				return;
			}

			DLOG_PRINT_DEBUG_MSG("Transformed image file saved at %s",
					image_util_filename_encoded);

		}
	}
	transform_finished = true;
}

/**
 * @brief Maps the image util color space to its string representation.
 *
 * @param color_space The image util color space, one of the image_util_colorspace_e values
 * @return The string representation of the image util color space
 */
static const char *_map_colorspace(image_util_colorspace_e color_space) {
	switch (color_space) {
	case IMAGE_UTIL_COLORSPACE_YV12:
		return "IMAGE_UTIL_COLORSPACE_YV12";

	case IMAGE_UTIL_COLORSPACE_YUV422:
		return "IMAGE_UTIL_COLORSPACE_YUV422";

	case IMAGE_UTIL_COLORSPACE_I420:
		return "IMAGE_UTIL_COLORSPACE_I420";

	case IMAGE_UTIL_COLORSPACE_NV12:
		return "IMAGE_UTIL_COLORSPACE_NV12";

	case IMAGE_UTIL_COLORSPACE_UYVY:
		return "IMAGE_UTIL_COLORSPACE_UYVY";

	case IMAGE_UTIL_COLORSPACE_YUYV:
		return "IMAGE_UTIL_COLORSPACE_YUYV";

	case IMAGE_UTIL_COLORSPACE_RGB565:
		return "IMAGE_UTIL_COLORSPACE_RGB565";

	case IMAGE_UTIL_COLORSPACE_RGB888:
		return "IMAGE_UTIL_COLORSPACE_RGB888";

	case IMAGE_UTIL_COLORSPACE_ARGB8888:
		return "IMAGE_UTIL_COLORSPACE_ARGB8888";

	case IMAGE_UTIL_COLORSPACE_BGRA8888:
		return "IMAGE_UTIL_COLORSPACE_BGRA8888";

	case IMAGE_UTIL_COLORSPACE_RGBA8888:
		return "IMAGE_UTIL_COLORSPACE_RGBA8888";

	case IMAGE_UTIL_COLORSPACE_BGRX8888:
		return "IMAGE_UTIL_COLORSPACE_BGRX8888";

	case IMAGE_UTIL_COLORSPACE_NV21:
		return "IMAGE_UTIL_COLORSPACE_NV21";

	case IMAGE_UTIL_COLORSPACE_NV16:
		return "IMAGE_UTIL_COLORSPACE_NV16";

	case IMAGE_UTIL_COLORSPACE_NV61:
		return "IMAGE_UTIL_COLORSPACE_NV61";
	}
}

/**
 * @brief Executes the image transformations.
 * @details Called once for each supported JPEG encode/decode color space.
 * @remarks This function matches the image_util_supported_jpeg_colorspace_cb()
 *          type signature defined in the Image Util API.
 *
 * @param color_space The color space
 * @param user_data The user data passed from the image_util_foreach_supported_jpeg_colorspace()
 *                  function (not used here)
 * @return @c true to continue with the next iteration of the loop,
 *         otherwise @c false to break out of the loop
 */
static bool _image_util_supported_jpeg_colorspace_cb(
		image_util_colorspace_e color_space, void *user_data) {
	DLOG_PRINT_DEBUG_MSG("%s", _map_colorspace(color_space));
	PRINT_MSG("%s", _map_colorspace(color_space));

	/* Continue the iteration over all supported color spaces. */
	return true;
}

/**
 * @brief Executes the image transformations.
 * @details Called when clicking any button from the Image Util (except
 *          the "Clear" button).
 * @remarks This function matches the Evas_Smart_Cb() type signature
 *          defined in the EFL API.
 *
 * @param data The button, which was clicked
 * @param object The object for which the 'clicked' event was triggered (not used here)
 * @param event_info Additional event information (not used here)
 */
static void _image_util_start_cb(void *data, Evas_Object *obj, void *event_info) {
	transform_finished = false;

	for (app_button i = 0; i < BUTTON_COUNT; ++i)
		_disable_button(i, EINA_TRUE);

	DIR* res;
	struct dirent* entry;
	struct stat buf;

	PRINT_MSG("Running transforming!");
	if ((res = opendir(resource_path)) == NULL) {
		DLOG_PRINT_ERROR("Cannot open resource_path", 0);
		PRINT_MSG("Cannot open resource_path");
		return;
	}
	while ((entry = readdir(res)) != NULL) {
		lstat(entry->d_name, &buf);
		if (S_ISDIR(buf.st_mode))
			continue;

		PRINT_MSG("img: %s", entry->d_name);
		/* Decode the given JPEG file to the img_source buffer. */
		unsigned char *img_source = NULL;
		int width, height;
		unsigned int size_decode;
		const char input_file_path[BUFLEN];
		snprintf(input_file_path, BUFLEN, "%s/%s", resource_path,
				entry->d_name);

		int error_code = image_util_decode_jpeg(input_file_path,
				IMAGE_UTIL_COLORSPACE_RGB888, &img_source, &width, &height,
				&size_decode);
		if (error_code != IMAGE_UTIL_ERROR_NONE) {
			PRINT_MSG("image_util_decode_jpeg() failed.");
			DLOG_PRINT_ERROR("image_util_decode_jpeg", error_code);
			return;
		}

		DLOG_PRINT_DEBUG_MSG("Decoded image width: %d height: %d size %d",
				width, height, size_decode);

		/* Create a media format structure. */
		media_format_h fmt;
		error_code = media_format_create(&fmt);
		if (error_code != MEDIA_FORMAT_ERROR_NONE) {
			PRINT_MSG("media_format_create() failed.");
			DLOG_PRINT_ERROR("media_format_create", error_code);
			free(img_source);
			return;
		}

		/* Set the MIME type of the created format. */
		error_code = media_format_set_video_mime(fmt, MEDIA_FORMAT_RGB888);
		if (error_code != MEDIA_FORMAT_ERROR_NONE) {
			PRINT_MSG("media_format_set_video_mime() failed.");
			DLOG_PRINT_ERROR("media_format_set_video_mime", error_code);
			media_format_unref(fmt);
			free(img_source);
			return;
		}

		/* Set the width of the created format. */
		error_code = media_format_set_video_width(fmt, width);
		if (error_code != MEDIA_FORMAT_ERROR_NONE) {
			PRINT_MSG("media_format_set_video_width() failed.");
			DLOG_PRINT_ERROR("media_format_set_video_width", error_code);
			media_format_unref(fmt);
			free(img_source);
			return;
		}

		/* Set the height of the created format. */
		error_code = media_format_set_video_height(fmt, height);
		if (error_code != MEDIA_FORMAT_ERROR_NONE) {
			PRINT_MSG("media_format_set_video_height() failed.");
			DLOG_PRINT_ERROR("media_format_set_video_height", error_code);
			media_format_unref(fmt);
			free(img_source);
			return;
		}

		/* Create a media packet with the image. */
		error_code = media_packet_create_alloc(fmt, NULL, NULL, &media_packet);
		if (error_code != MEDIA_PACKET_ERROR_NONE) {
			PRINT_MSG("media_packet_create_alloc() failed.");
			DLOG_PRINT_ERROR("media_packet_create_alloc", error_code);
			media_format_unref(fmt);
			free(img_source);
			return;
		}

		media_format_unref(fmt);

		/* Get the pointer to the internal media packet buffer, where the image will be stored. */
		void *packet_buffer = NULL;

		error_code = media_packet_get_buffer_data_ptr(media_packet,
				&packet_buffer);
		if (error_code != MEDIA_PACKET_ERROR_NONE || NULL == packet_buffer) {
			PRINT_MSG("media_packet_get_buffer_data_ptr() failed.");
			DLOG_PRINT_ERROR("media_packet_get_buffer_data_ptr", error_code);
			free(img_source);
			return;
		}

		/* Copy the image content to the media_packet internal buffer. */
		memcpy(packet_buffer, (void *) img_source, size_decode);
		free(img_source);

		/* Create a handle to the transformation. */
		error_code = image_util_transform_create(&handle);
		if (error_code != IMAGE_UTIL_ERROR_NONE) {
			PRINT_MSG("image_util_transform_create() failed.");
			DLOG_PRINT_ERROR("image_util_transform_create", error_code);
			return;
		}

		/* Disable the hardware acceleration for the created transformation. */
		error_code = image_util_transform_set_hardware_acceleration(handle,
		false);
		CHECK_ERROR("image_util_transform_set_hardware_acceleration",
				error_code);

		PRINT_MSG("<b>Converting the image color space.</b>");
		DLOG_PRINT_DEBUG_MSG("Converting the image color space.");

		image_util_colorspace_e colorspace = IMAGE_UTIL_COLORSPACE_NV12;

		/* Set the color space the image color space will be converted to. */
		error_code = image_util_transform_set_colorspace(handle, colorspace);
		if (error_code != IMAGE_UTIL_ERROR_NONE) {
			PRINT_MSG("image_util_transform_set_colorspace() failed.");
			DLOG_PRINT_ERROR("image_util_transform_set_colorspace", error_code);
			return;
		}
		PRINT_MSG("Color space set to %s", _map_colorspace(colorspace));

		/* Set new values for the width and height the image will be resized to. */
		unsigned int new_width = atoi(elm_entry_entry_get(s_info.width));
		unsigned int new_height = atoi(elm_entry_entry_get(s_info.height));

		error_code = image_util_transform_set_resolution(handle, new_width,
				new_height);
		if (error_code != IMAGE_UTIL_ERROR_NONE) {
			PRINT_MSG("image_util_transform_set_resolution() failed.");
			DLOG_PRINT_ERROR("image_util_transform_set_resolution", error_code);
			return;
		}

		PRINT_MSG("New resolution is:%dx%d", new_width, new_height);

		snprintf(image_util_filename_encoded, BUFLEN, "%s/%s", images_directory,
				entry->d_name);

		/* Execute the transformation. */
		error_code = image_util_transform_run(handle, media_packet,
				_image_util_completed_cb, NULL);
		if (error_code != IMAGE_UTIL_ERROR_NONE) {
			PRINT_MSG("image_util_transform_run() failed.");
			DLOG_PRINT_ERROR("image_util_transform_run", error_code);
		}
	}
}

/**
 * @brief Assigns the ID of the internal storage to the variable passed
 *        as the user data to the callback.
 * @details Called for every storage supported by the device.
 * @remarks This function matches the storage_device_supported_cb() signature
 *          defined in the storage-expand.h header file.
 *
 * @param storage_id  The unique ID of the detected storage
 * @param type        The type of the detected storage
 * @param state       The current state of the detected storage.
 *                    This argument is not used in this case.
 * @param path        The absolute path to the root directory of the detected
 *                    storage. This argument is not used in this case.
 * @param user_data   The user data passed via void pointer
 *
 * @return @c true to continue iterating over supported storages, @c false to
 *         stop the iteration.
 */
static bool _storage_cb(int storage_id, storage_type_e type,
		storage_state_e state, const char *path, void *user_data) {
	if (STORAGE_TYPE_INTERNAL == type) {
		int *internal_storage_id = (int *) user_data;
		*internal_storage_id = storage_id;

		/* Internal storage found, stop the iteration. */
		return false;
	} else {
		/* Continue iterating over storages. */
		return true;
	}
}

/**
 * @brief Creates the application main view.
 */
void create_buttons_in_main_window() {
	/* Create the window for the Image Util. */
	Evas_Object *display = _create_new_cd_display("Image Util", NULL);

	/* Create buttons for the Image Util. */
	_create_button(CONVERT_BTN, display, "Convert the Color Space",
			_image_util_start_cb);

	/* Get the path to the resources. */
	resource_path = app_get_resource_path();

	/* Get the path to the Images directory: */

	/* 1. Get internal storage id. */
	int internal_storage_id = -1;

	int error_code = storage_foreach_device_supported(_storage_cb,
			&internal_storage_id);
	if (STORAGE_ERROR_NONE != error_code) {
		DLOG_PRINT_ERROR("storage_foreach_device_supported", error_code);
		free(resource_path);
		return;
	}

	/* 2. Get the path to the Images directory. */
	error_code = storage_get_directory(internal_storage_id,
			STORAGE_DIRECTORY_IMAGES, &images_directory);
	CHECK_ERROR("storage_get_directory", error_code);

	ecore_idler_add(_btn_enable, NULL);
}

/**
 * @brief Loads the image which will be used as a source to the display.
 * @details Called when the 'Clear' button is clicked.
 * @remarks This function matches the Evas_Smart_Cb() type signature
 *          defined in the EFL API.
 *
 * @param data The user data passed via void pointer (not used here)
 * @param object The object for which the 'clicked' event was triggered (not used here)
 * @param event_info Additional event information (not used here)
 */
void _image_util_clear_cb(void *data, Evas_Object *obj, void *event_info) {
	for (app_button i = 0; i < BUTTON_COUNT; ++i)
		_disable_button(i, EINA_FALSE);
}
