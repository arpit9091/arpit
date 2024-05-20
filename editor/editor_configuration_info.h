/**************************************************************************/
/*  editor_configuration_info.h                                           */
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

#ifndef EDITOR_CONFIGURATION_INFO_H
#define EDITOR_CONFIGURATION_INFO_H

#include "core/object/ref_counted.h"
#include "core/string/ustring.h"

class EditorConfigurationInfo {
public:
	EditorConfigurationInfo() {}

	static Array get_configuration_info_dicts(Object *p_object);
	static String get_max_severity(const Array &p_config_info_dicts);
	static String get_severity_icon(const String &p_severity);

	static Dictionary convert_string_to_dict(const String &p_config_info_string);
	static Array filter_dict_list_for_property(const Array &p_config_info_dicts, const String &p_property_name);

	static String format_dict_list_as_string(const Array &p_config_info_dicts, bool p_wrap_lines, bool p_prefix_property_name);

private:
	static Array convert_mixed_array_to_dict(const Array &p_mixed_config_infos);
};

#endif // EDITOR_CONFIGURATION_INFO_H
