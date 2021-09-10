#include <stdio.h>
#include <string>
#include <iostream>     // std::cout
#include <fstream>      // std::ifstream
#include <vector>
#include <sstream>
#include <algorithm>    // std::min

enum EOL_TYPE {
  EOL_TYPE_UNIX,
  EOL_TYPE_WINDOWS
};

// Type definitions and structures
typedef std::string HtmlTableCell;
typedef std::vector<HtmlTableCell> HtmlTableRow;
struct HtmlTable {
  bool first_row_is_header;
  std::vector<HtmlTableRow> rows;
};

struct HTML_TAG_INFO {
  size_t open_start;
  size_t open_end;
  size_t inner_start;
  size_t inner_end;
  size_t close_start;
  size_t close_end;
};

struct HTML_ATTRIBUTE_INFO {
  size_t attr_value_start;
  size_t attr_value_end;
};

#define HTML_TAG_A          0x00000001
#define HTML_TAG_BR         0x00000002
#define HTML_TAG_SUB        0x00000004
#define HTML_TAG_SUP        0x00000008
#define HTML_TAG_COMMENT    0x00000010

// declarations
bool is_digit(const char c);
bool is_letter(const char c);
bool is_alphanumeric(const char c);
bool is_numeric(const char * value);
size_t count_character(const char * text, char character);
size_t get_line_indentation(const char * content, size_t offset);
size_t string_length_utf8(const char * text);
size_t string_length_utf8(const std::string & text);
size_t string_length(const std::string & text);
std::string string_center(const std::string & text, size_t length);
std::string string_lengthen(const std::string & text, size_t length);
size_t get_table_column_count(const HtmlTable & table);
size_t get_table_column_width(const HtmlTable & table, size_t column_index);
std::string to_markdown(const HtmlTable & table);
EOL_TYPE normalize_newlines(std::string & content);
void restore_newlines(std::string & content, EOL_TYPE eol_type);
void search_and_replace(std::string & content, const std::string & token, const std::string & value);
std::string load_file(const std::string & path);
bool save_file(const std::string & path, const std::string & content);
bool file_exists(const std::string & name);
bool find_html_tag_boundaries(const std::string & content, const std::string & tag_name, size_t offset, HTML_TAG_INFO & info);
bool find_html_attribute_boundaries(const std::string & content, const std::string & attr_name, size_t offset_start, size_t offset_end, HTML_ATTRIBUTE_INFO & info);
std::string get_html_attribute_value(const std::string & content, const std::string & attr_name, const HTML_TAG_INFO & tag_info);
std::string find_class_property(const std::string & class_value, const std::string & property_name);
size_t get_html_tag_indentation(const char * content, const HTML_TAG_INFO & tag_info);
bool is_any(char c, const std::vector<char> & characters);
bool is_any(char c, const char * characters, size_t num_characters);
std::string read_until(const std::string & content, size_t offset, const std::vector<char> & stop_characters);
std::string read_until(const std::string & content, size_t offset, const char * characters, size_t num_characters);
std::string read_html_tag(const std::string & content, size_t offset);
bool has_inner_html_tags(const std::string & content, int ignore_html_tag_flags = HTML_TAG_BR|HTML_TAG_SUB|HTML_TAG_SUP|HTML_TAG_COMMENT);
bool has_inner_html_tags(const HtmlTableRow & row, int ignore_html_tag_flags = HTML_TAG_BR|HTML_TAG_SUB|HTML_TAG_SUP|HTML_TAG_COMMENT);
bool has_inner_html_tags(const HtmlTable & table, int ignore_html_tag_flags = HTML_TAG_BR|HTML_TAG_SUB|HTML_TAG_SUP|HTML_TAG_COMMENT);
bool has_cell_spanning(const std::string & content);
std::vector<std::string> split(const std::string & content, char split_character);
std::string join(const std::vector<std::string> & lines, char join_character);
size_t trim(std::string & text, char trim_character);
size_t trim(std::string & text, const char * trim_str);
size_t trim_html_whitespace(std::string & text);
size_t trim_html_whitespace(HtmlTable & table);
bool is_html_white_character(const std::string & content, size_t offset);
size_t get_first_nonwhite_html_character(const char * content, size_t offset);
void decrease_indent(std::vector<std::string> & lines, size_t num_spaces);
std::string get_line_at_offset(std::string & content, size_t offset);
bool is_custom_css_class(const std::string & class_);
bool is_division_gallery(const std::string & content, const HTML_TAG_INFO & info);
bool parse_gallery_table(std::string & content, HtmlTable & table);
bool parse_html_table(std::string & content, HtmlTable & table);
bool file_exists(const char * path);
bool dir_exists(const char * path);
std::string get_env_variable(const char * name);
std::string get_temp_directory();
std::string get_file_separator();
std::string find_argument(const char * name, int argc, char* argv[]);
std::vector<std::string> get_files_in_directory(const char * directory);
std::vector<std::string> read_file_lines(const char * path);
std::string get_parent_directory(const char * path);
std::string get_file_name(const char * path);
std::string get_file_extension(const char * path);
std::string get_file_name_with_extension(const char * path);
bool is_sub_image_size(const char * master_path, const char * test_path);
void uppercase(std::string & str);
bool delete_file(const std::string & path);

// Functions
template <class T>
std::string to_string(const T & t) {
  std::stringstream out;
  out << t;
  const std::string & s = out.str();
  return s;
}

template <class T>
void parse_value(const std::string & value, T & t) {
  std::istringstream input_stream(value);
  input_stream >> t;
}
