/**************************************************************************/
/*  lottie_texture.h                                                      */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/
/* Copyright (c) 2014-present Godot Engine contributors (see AUTHORS.md). */
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                  */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

#ifndef LOTTIE_TEXTURE_H
#define LOTTIE_TEXTURE_H

#include "core/io/json.h"
#include "scene/resources/texture.h"

#include <thorvg.h>

class LottieTexture2D : public Texture2D {
	GDCLASS(LottieTexture2D, Texture2D);

	std::unique_ptr<tvg::SwCanvas> sw_canvas = tvg::SwCanvas::gen();
	std::unique_ptr<tvg::Animation> animation = tvg::Animation::gen();
	tvg::Picture *picture = animation->picture();
	Ref<Image> image;
	mutable RID texture;
	uint32_t *buffer = nullptr;
	Ref<JSON> json = nullptr;

	float scale = 1.0;
	float origin_width = -1, origin_height = -1;

	float frame_begin = 0;
	float frame_end = 0;
	int frame_count = 1;
	int rows = -1;

	void _load_lottie_json();
	void _update_image();

protected:
	static void _bind_methods();

public:
	static Ref<LottieTexture2D> create_from_string(String p_string, float p_frame_begin = 0, float p_frame_end = 0, int p_frame_count = 1, float p_scale = 1, int p_rows = -1);
	static Ref<LottieTexture2D> create_from_json(Ref<JSON> p_json, float p_frame_begin = 0, float p_frame_end = 0, int p_frame_count = 1, float p_scale = 1, int p_rows = -1);

	void update(Ref<JSON> p_json, float p_frame_begin, float p_frame_end, int p_frame_count, float p_scale, int p_rows);

	void set_json(Ref<JSON> p_json);
	Ref<JSON> get_json() { return json; };

	void set_scale(float p_scale);
	float get_scale() { return scale; };

	void set_frame_begin(float p_frame_begin);
	float get_frame_begin() { return frame_begin; };

	void set_frame_end(float p_frame_end);
	float get_frame_end() { return frame_end; };

	void set_frame_count(int p_frame_count);
	int get_frame_count() { return frame_count; };

	void set_rows(int p_rows);
	int get_rows() { return rows; }

	float get_lottie_duration() { return animation->duration(); };
	float get_lottie_frame_count() { return animation->totalFrame(); };
	Size2 get_lottie_image_size() {
		float w, h;
		picture->size(&w, &h);
		return Size2(w, h);
	}

	int get_width() const override { return image.is_valid() ? image->get_width() : 0; };
	int get_height() const override { return image.is_valid() ? image->get_height() : 0; };
	Size2 get_size() const override { return image.is_valid() ? image->get_size() : Size2i(); };
	virtual bool is_pixel_opaque(int p_x, int p_y) const override { return image.is_valid() ? image->get_pixel(p_x, p_y).a > 0.1 : true; };
	virtual bool has_alpha() const override { return true; };
	virtual Ref<Image> get_image() const override { return image; };
	virtual RID get_rid() const override;

	~LottieTexture2D();
};

class ResourceFormatLoaderLottie : public ResourceFormatLoader {
public:
	virtual Ref<Resource> load(const String &p_path, const String &p_original_path = "", Error *r_error = nullptr, bool p_use_sub_threads = false, float *r_progress = nullptr, CacheMode p_cache_mode = CACHE_MODE_REUSE) override;
	virtual void get_recognized_extensions(List<String> *p_extensions) const override;
	virtual bool handles_type(const String &p_type) const override;
	virtual String get_resource_type(const String &p_path) const override;
};

class ResourceFormatSaverLottie : public ResourceFormatSaver {
public:
	virtual Error save(const Ref<Resource> &p_resource, const String &p_path, uint32_t p_flags = 0) override;
	virtual void get_recognized_extensions(const Ref<Resource> &p_resource, List<String> *p_extensions) const override;
	virtual bool recognize(const Ref<Resource> &p_resource) const override;
};

#endif // LOTTIE_TEXTURE_H
