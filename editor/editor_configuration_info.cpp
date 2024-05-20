/**************************************************************************/
/*  editor_configuration_info.cpp                                         */
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

#include "editor_configuration_info.h"

#include "core/io/resource.h"
#include "scene/main/node.h"
#include "servers/text_server.h"

Array EditorConfigurationInfo::get_configuration_info_dicts(Object *p_object) {
	Array config_info;
	if (!p_object) {
		return config_info;
	}

	Node *node = Object::cast_to<Node>(p_object);
	if (node) {
		config_info.append_array(node->get_configuration_info());
#ifndef DISABLE_DEPRECATED
		PackedStringArray warnings = node->get_configuration_warnings();
		for (int i = 0; i < warnings.size(); i++) {
			config_info.push_back(warnings[i]);
		}
#endif
	} else {
		Resource *resource = Object::cast_to<Resource>(p_object);
		if (resource) {
			config_info.append_array(resource->get_configuration_info());
		} else {
			// Unable to get config info from anything except Nodes and Resources.
			return config_info;
		}
	}
	return convert_mixed_array_to_dict(config_info);
}

String EditorConfigurationInfo::get_max_severity(const Array &p_config_info_dicts) {
	String max_severity;

	for (int i = 0; i < p_config_info_dicts.size(); i++) {
		Dictionary config_info = p_config_info_dicts[i];
		String severity = config_info.get("severity", "warning");
		// "info" is the lowest, "error" is the highest, "warning" can only override "info".
		if (max_severity.is_empty() || severity == "error" || (severity == "warning" && max_severity != "error")) {
			max_severity = severity;
		}
	}

	return max_severity;
}

String EditorConfigurationInfo::get_severity_icon(const String &p_severity) {
	if (p_severity == "error") {
		return "StatusError";
	} else if (p_severity == "warning") {
		return "NodeWarning";
	} else if (p_severity == "info") {
		return "NodeInfo";
	} else {
		return "";
	}
}

Dictionary EditorConfigurationInfo::convert_string_to_dict(const String &p_config_info_string) {
	Dictionary config_info;
	config_info["message"] = p_config_info_string;
	return config_info;
}

Array EditorConfigurationInfo::filter_dict_list_for_property(const Array &p_config_info_dicts, const String &p_property_name) {
	Array result;
	for (int i = 0; i < p_config_info_dicts.size(); i++) {
		Dictionary dict = p_config_info_dicts[i];
		if (dict.get("property", "") == p_property_name) {
			result.push_back(dict);
		}
	}
	return result;
}

String EditorConfigurationInfo::format_dict_list_as_string(const Array &p_config_info_dicts, bool p_wrap_lines, bool p_prefix_property_name) {
	const String bullet_point = U"•  ";
	PackedStringArray all_lines;
	for (int i = 0; i < p_config_info_dicts.size(); i++) {
		Dictionary config_info = p_config_info_dicts[i];
		if (!config_info.has("message")) {
			continue;
		}

		String text;
		String property_name = config_info.get("property", "");
		if (p_prefix_property_name && !property_name.is_empty()) {
			text = bullet_point + vformat("[%s] %s", config_info["property"], config_info["message"]);
		} else {
			text = bullet_point + static_cast<String>(config_info["message"]);
		}

		if (p_wrap_lines) {
			// Limit the line width while keeping some padding.
			// It is not efficient, but it does not have to be.
			const PackedInt32Array boundaries = TS->string_get_word_breaks(text, "", 80);
			PackedStringArray lines;
			for (int bound = 0; bound < boundaries.size(); bound += 2) {
				const int start = boundaries[bound];
				const int end = boundaries[bound + 1];
				String line = text.substr(start, end - start);
				lines.append(line);
			}
			text = String("\n").join(lines);
		}
		text = text.replace("\n", "\n    ");
		all_lines.append(text);
	}
	return String("\n").join(all_lines);
}

Array EditorConfigurationInfo::convert_mixed_array_to_dict(const Array &p_mixed_config_infos) {
	Array result;
	for (int i = 0; i < p_mixed_config_infos.size(); i++) {
		Variant item = p_mixed_config_infos[i];
		if (item.get_type() == Variant::STRING) {
			String str = item;
			result.push_back(convert_string_to_dict(str));
		} else if (item.get_type() == Variant::DICTIONARY) {
			Dictionary dict = item;
			result.push_back(dict);
		} else {
			print_error("get_configuration_info returned a value which is neither a string nor a dictionary, but a " + Variant::get_type_name(item.get_type()));
		}
	}
	return result;
}
