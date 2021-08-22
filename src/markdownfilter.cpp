// markdownfilter.cpp : Defines the entry point for the console application.
//

#include <stdio.h>
#include <string>
#include <iostream>     // std::cout
#include <fstream>      // std::ifstream
#include <vector>
#include <sstream>
#include <algorithm>    // std::min

static std::string WEBSITE_HOSTNAME = "http://www.end2endzone.com";
static bool process_file_in_place = true;

static bool is_utf8_source = true; //change this to false if you are parsing from Windows code page 1252 (Cp1252) or from ISO8859-1 character encoding

static const char html_tag_endding_characters[] = { ' ', '/', '\"', '\'', '>', '\0' };
static const size_t num_html_tag_endding_characters = sizeof(html_tag_endding_characters) / sizeof(html_tag_endding_characters[0]);

static const char link_reference_endding_characters[] = { '\n', '\0' };
static const size_t num_link_reference_endding_characters = sizeof(link_reference_endding_characters) / sizeof(link_reference_endding_characters[0]);

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

// Pre-declarations
inline bool is_digit(const char c);
inline bool is_letter(const char c);
inline bool is_alphanumeric(const char c);
inline bool is_numeric(const char * value);
inline size_t count_character(const char * text, char character);
inline size_t get_line_indentation(const char * content, size_t offset);
inline size_t string_length_utf8(const char * text);
inline size_t string_length_utf8(const std::string & text);
inline size_t string_length(const std::string & text);
inline std::string string_center(const std::string & text, size_t length);
inline std::string string_lengthen(const std::string & text, size_t length);
inline size_t get_table_column_count(const HtmlTable & table);
inline size_t get_table_column_width(const HtmlTable & table, size_t column_index);
inline std::string to_markdown(const HtmlTable & table);
inline EOL_TYPE normalize_newlines(std::string & content);
inline void restore_newlines(std::string & content, EOL_TYPE eol_type);
inline void search_and_replace(std::string & content, const std::string & token, const std::string & value);
inline std::string load_file(const std::string & path);
inline bool save_file(const std::string & path, const std::string & content);
inline bool file_exists(const std::string & name);
bool find_html_tag_boundaries(const std::string & content, const std::string & tag_name, size_t offset, HTML_TAG_INFO & info);
bool find_html_attribute_boundaries(const std::string & content, const std::string & attr_name, size_t offset_start, size_t offset_end, HTML_ATTRIBUTE_INFO & info);
inline std::string get_html_attribute_value(const std::string & content, const std::string & attr_name, const HTML_TAG_INFO & tag_info);
std::string find_class_property(const std::string & class_value, const std::string & property_name);
inline size_t get_html_tag_indentation(const char * content, const HTML_TAG_INFO & tag_info);
inline bool is_any(char c, const std::vector<char> & characters);
inline bool is_any(char c, const char * characters, size_t num_characters);
inline std::string read_until(const std::string & content, size_t offset, const std::vector<char> & stop_characters);
inline std::string read_until(const std::string & content, size_t offset, const char * characters, size_t num_characters);
inline std::string read_html_tag(const std::string & content, size_t offset);
inline bool has_inner_html_tags(const std::string & content, int ignore_html_tag_flags = HTML_TAG_BR|HTML_TAG_SUB|HTML_TAG_SUP|HTML_TAG_COMMENT);
inline bool has_inner_html_tags(const HtmlTableRow & row, int ignore_html_tag_flags = HTML_TAG_BR|HTML_TAG_SUB|HTML_TAG_SUP|HTML_TAG_COMMENT);
inline bool has_inner_html_tags(const HtmlTable & table, int ignore_html_tag_flags = HTML_TAG_BR|HTML_TAG_SUB|HTML_TAG_SUP|HTML_TAG_COMMENT);
inline bool has_cell_spanning(const std::string & content);
inline std::vector<std::string> split(const std::string & content, char split_character);
inline std::string join(const std::vector<std::string> & lines, char join_character);
inline size_t trim(std::string & text, char trim_character);
inline size_t trim(std::string & text, const char * trim_str);
inline size_t trim_html_whitespace(std::string & text);
inline size_t trim_html_whitespace(HtmlTable & table);
inline bool is_html_white_character(const std::string & content, size_t offset);
inline size_t get_first_nonwhite_html_character(const char * content, size_t offset);
inline void decrease_indent(std::vector<std::string> & lines, size_t num_spaces);
inline std::string get_line_at_offset(std::string & content, size_t offset);
inline bool is_custom_css_class(const std::string & class_);
inline bool is_division_gallery(const std::string & content, const HTML_TAG_INFO & info);
bool parse_gallery_table(std::string & content, HtmlTable & table);
bool parse_html_table(std::string & content, HtmlTable & table);

void filter_span(std::string & content);
void filter_paragraph(std::string & content);
void filter_images(std::string & content);
void filter_anchors(std::string & content);
void filter_strong(std::string & content);
void filter_italic(std::string & content);
void filter_emphasized(std::string & content);
void filter_code(std::string & content);
void filter_unordered_lists(std::string & content);
void filter_list_item(std::string & content);
void filter_list_item_simplify(std::string & content);
void filter_preformatted_code_simplify(std::string & content);
void filter_division(std::string & content);
void filter_division_gallery(std::string & content);
void filter_table(std::string & content);
void filter_preformatted(std::string & content);
void filter_known_html_entities(std::string & content);
void filter_useless_nbsp_entities(std::string & content);
void filter_table_cells_inner_white_space(std::string & content);
void filter_table_cells_outer_white_space(std::string & content);
void filter_table_rows_outer_white_space(std::string & content);
void force_inline_hyperlinks(std::string & content);
void filter_missing_newline(std::string & content, const std::string & tag_close_definition);
void filter_missing_newline(std::string & content);
void filter_missing_newline_after_header(std::string & content);
void filter_featured_image(std::string & content);
void filter_comment_separators(std::string & content);
void filter_type_post(std::string & content);
void filter_more_comment(std::string & content);

void run_all_filters(std::string & content);


// Functions
template <class T>
inline std::string to_string(const T & t) {
  std::stringstream out;
  out << t;
  const std::string & s = out.str();
  return s;
}

template <class T>
inline void parse_value(const std::string & value, T & t) {
  std::istringstream input_stream(value);
  input_stream >> t;
}

inline bool is_digit(const char c) {
  return (c >= '0' && c <= '9');
}

inline bool is_letter(const char c) {
  return (  (c >= 'a' && c <= 'z') ||
            (c >= 'A' && c <= 'Z')  );
}

inline bool is_alphanumeric(const char c) {
  return (is_digit(c) || is_letter(c));
}

inline bool is_numeric(const char * value) {
  // from https://raw.githubusercontent.com/end2endzone/RapidAssist/master/src/rapidassist/strings.cpp
  if (value == NULL)
    return false;

  bool found_dot = false;
  size_t length = strlen(value);
  for (size_t offset = 0; offset < length; offset++) {
    const char & c = value[offset];
    if (c >= '0' && c <= '9')
      continue; //valid
    else if (c == '.' && !found_dot) {
      //only 1 dot character must be found in the string
      found_dot = true;
      continue; //valid
    }
    else if ((c == '+' || c == '-')) {
      if (offset == 0) {
        //+ or - sign are accepted but must be the first character of the value
        continue; //valid
      }
    }

    return false; //invalid
  }
  return true;
}

inline size_t count_character(const char * text, char character) {
  size_t count = 0;
  while(text[0] != '\0') {
    if (text[0] == character)
      count++;
    text++;
  }
  return count;
}

inline size_t get_line_indentation(const char * content, size_t offset) {
  size_t indent = 0;
  while(offset != std::string::npos && content[offset] != '\n') {
    if (content[offset] == ' ')
      indent++;
    else
      indent = 0;

    offset--;
  }
  return indent;
}

inline size_t string_length_utf8(const char * text) {
  // from https://raw.githubusercontent.com/end2endzone/RapidAssist/master/src/rapidassist/unicode.cpp

  size_t code_points = 0;

  const unsigned char * unsigned_str = (const unsigned char *)text;
  int offset = 0;
  while (text[offset] != '\0')
  {
    const unsigned char & c1 = unsigned_str[offset + 0];
    unsigned char c2 = unsigned_str[offset + 1];
    unsigned char c3 = unsigned_str[offset + 2];
    unsigned char c4 = unsigned_str[offset + 3];
    
    //prevent going outside of the string
    if (c1 == '\0')
      c2 = c3 = c4 = '\0';
    else if (c2 == '\0')
      c3 = c4 = '\0';
    else if (c3 == '\0')
      c4 = '\0';

    //size in bytes of the code point
    int n = 1;

    //See http://www.unicode.org/versions/Unicode6.0.0/ch03.pdf, Table 3-7. Well-Formed UTF-8 Byte Sequences
    // ## | Code Points         | First Byte | Second Byte | Third Byte | Fourth Byte
    // #1 | U+0000   - U+007F   | 00 - 7F    |             |            | 
    // #2 | U+0080   - U+07FF   | C2 - DF    | 80 - BF     |            | 
    // #3 | U+0800   - U+0FFF   | E0         | A0 - BF     | 80 - BF    | 
    // #4 | U+1000   - U+CFFF   | E1 - EC    | 80 - BF     | 80 - BF    | 
    // #5 | U+D000   - U+D7FF   | ED         | 80 - 9F     | 80 - BF    | 
    // #6 | U+E000   - U+FFFF   | EE - EF    | 80 - BF     | 80 - BF    | 
    // #7 | U+10000  - U+3FFFF  | F0         | 90 - BF     | 80 - BF    | 80 - BF
    // #8 | U+40000  - U+FFFFF  | F1 - F3    | 80 - BF     | 80 - BF    | 80 - BF
    // #9 | U+100000 - U+10FFFF | F4         | 80 - 8F     | 80 - BF    | 80 - BF

    if (c1 <= 0x7F) // #1 | U+0000   - U+007F, (ASCII)
      n = 1;
    else if ( 0xC2 <= c1 && c1 <= 0xDF &&
              0x80 <= c2 && c2 <= 0xBF)  // #2 | U+0080   - U+07FF
      n = 2;
    else if ( 0xE0 == c1 &&
              0xA0 <= c2 && c2 <= 0xBF &&
              0x80 <= c3 && c3 <= 0xBF)  // #3 | U+0800   - U+0FFF
      n = 3;
    else if ( 0xE1 <= c1 && c1 <= 0xEC &&
              0x80 <= c2 && c2 <= 0xBF &&
              0x80 <= c3 && c3 <= 0xBF)  // #4 | U+1000   - U+CFFF
      n = 3;
    else if ( 0xED == c1 &&
              0x80 <= c2 && c2 <= 0x9F &&
              0x80 <= c3 && c3 <= 0xBF)  // #5 | U+D000   - U+D7FF
      n = 3;
    else if ( 0xEE <= c1 && c1 <= 0xEF &&
              0x80 <= c2 && c2 <= 0xBF &&
              0x80 <= c3 && c3 <= 0xBF)  // #6 | U+E000   - U+FFFF
      n = 3;
    else if ( 0xF0 == c1 &&
              0x90 <= c2 && c2 <= 0xBF &&
              0x80 <= c3 && c3 <= 0xBF &&
              0x80 <= c4 && c4 <= 0xBF)  // #7 | U+10000  - U+3FFFF
      n = 4;
    else if ( 0xF1 <= c1 && c1 <= 0xF3 &&
              0x80 <= c2 && c2 <= 0xBF &&
              0x80 <= c3 && c3 <= 0xBF &&
              0x80 <= c4 && c4 <= 0xBF)  // #8 | U+40000  - U+FFFFF
      n = 4;
    else if ( 0xF4 == c1 &&
              0x80 <= c2 && c2 <= 0xBF &&
              0x80 <= c3 && c3 <= 0xBF &&
              0x80 <= c4 && c4 <= 0xBF)  // #7 | U+10000  - U+3FFFF
      n = 4;
    else
      return 0; // invalid UTF-8 sequence

    code_points++;

    //next code point
    offset += n;
  }
  return code_points;
}
inline size_t string_length_utf8(const std::string & text) {
  return string_length_utf8(text.c_str());
}

inline size_t string_length(const std::string & text) {
  size_t length = 0;
  if (is_utf8_source)
    length = string_length_utf8(text);
  else
    length = text.size();
  return length;
}

inline std::string string_center(const std::string & text, size_t length) {
  std::string output = text;
  output.reserve(length);

  if (length % 2 == 1)
    length++;

  while(string_length(output) < length) {
    // insert a space on the left and right of the text
    output.insert(0, 1, ' ');
    output.append(1, ' ');
  }

  return output;
}

inline std::string string_lengthen(const std::string & text, size_t length) {
  std::string output = text;
  output.reserve(length);
  size_t text_length = string_length(text);
  if (text_length < length) {
    size_t diff = length - text_length;
    output.append(diff, ' ');
  }
  return output;
}

inline size_t get_table_column_count(const HtmlTable & table) {
  size_t columns = 0;
  for(size_t i=0; i<table.rows.size(); i++) {
    const HtmlTableRow & row = table.rows[i];
    if (columns < row.size())
      columns = row.size();
  }
  return columns;
}

inline size_t get_table_column_width(const HtmlTable & table, size_t column_index) {
  size_t width = std::string::npos;
  for(size_t i=0; i<table.rows.size(); i++) {
    const HtmlTableRow & row = table.rows[i];
    if (column_index < row.size()) {
      const HtmlTableCell & cell = row[column_index];
      size_t cell_width = string_length(cell);
      if (width == std::string::npos || width < cell_width) {
        width = cell_width;
      }
    }
  }
  return width;
}

inline std::string to_markdown(const HtmlTable & table) {
  std::string markdown;

  if ((table.first_row_is_header  && table.rows.size() <= 1) || 
      (!table.first_row_is_header && table.rows.size() == 0)  )
    return std::string();

  size_t num_columns = get_table_column_count(table);
  if (num_columns == 0)
    return std::string();

  // compute the width of each column
  std::vector<size_t> columns_width;
  for(size_t i=0; i<num_columns; i++) {
    size_t size = get_table_column_width(table, i);
    if (size == std::string::npos) {
      break;
    }
    columns_width.push_back(size);
  }

  // increase length of columns for empty cells, if required
  static const std::string empty_header_cell_content = "<!-- -->";
  static const size_t empty_header_cell_content_length = string_length(empty_header_cell_content);
  if (!table.first_row_is_header) {
    for(size_t i=0; i<num_columns; i++) {
      if (columns_width[i] < empty_header_cell_content_length)
        columns_width[i] = empty_header_cell_content_length;
    }
  }

  // create a first dummy header, may not be always required
  HtmlTableRow dummy;
  for(size_t i=0; i<num_columns; i++) {
    dummy.push_back(empty_header_cell_content);
  }

  // select header row for the header
  const HtmlTableRow * header = NULL;
  size_t first_data_row_index = std::string::npos;
  if (!table.first_row_is_header) {
    header = &dummy;
    first_data_row_index = 0;
  } else {
    header = &(table.rows[0]);
    first_data_row_index = 1;
  }

  // print header
  for(size_t i=0; i<num_columns; i++) {
    const size_t & width = columns_width[i];
    markdown.append("| ");
    HtmlTableCell cell;
    if (i < header->size())
      cell = (*header)[i];
    trim_html_whitespace(cell);
    markdown.append(string_lengthen(cell, width));
    markdown.append(" ");
  }
  markdown.append("|\n");

  // print dashes
  for(size_t i=0; i<num_columns; i++) {
    const size_t & width = columns_width[i];
    markdown.append("|-");
    std::string dashes(width, '-');
    markdown.append(dashes);
    markdown.append("-");
  }
  markdown.append("|\n");

  // print data rows
  for(size_t i=first_data_row_index; i<table.rows.size(); i++) {
    const HtmlTableRow & row = table.rows[i];
    for(size_t j=0; j<num_columns; j++) {
      const size_t & width = columns_width[j];
      HtmlTableCell cell;
      if (j < row.size())
        cell = row[j];
      trim_html_whitespace(cell);
      markdown.append("| ");
      markdown.append(string_lengthen(cell, width));
      markdown.append(" ");
    }
    markdown.append("|\n");
  }

  return markdown;
}

inline EOL_TYPE normalize_newlines(std::string & content) {
  size_t lf_count = count_character(content.c_str(), '\n');
  size_t cr_count = count_character(content.c_str(), '\r');

  // compute how many os specific newline there are.
  size_t unix_newline     = lf_count-cr_count;
  size_t windows_newline  = cr_count;

  EOL_TYPE eol_type = EOL_TYPE_UNIX;

  if (unix_newline > 0 && windows_newline == 0) {
    printf("The document uses unix EOL.\n");
    eol_type = EOL_TYPE_UNIX;
  }
  else if (windows_newline > 0 && unix_newline == 0) {
    printf("The document uses windows EOL.\n");
    eol_type = EOL_TYPE_WINDOWS;
  }
  else if (unix_newline >= windows_newline) {
    printf("The document uses unix EOL (mostly).\n");
    eol_type = EOL_TYPE_UNIX;
  }
  else if (windows_newline >= unix_newline) {
    printf("The document uses windows EOL (mostly).\n");
    eol_type = EOL_TYPE_WINDOWS;
  }

  // Force unix newlines while processing.

  // Remove window newlines in the document
  search_and_replace(content, "\r\n", "\n");

  return eol_type;
}

inline void restore_newlines(std::string & content, EOL_TYPE eol_type) {
  switch (eol_type) {
  case EOL_TYPE_UNIX:
    // keep unix newlines
    // remove window newlines in the document.
    search_and_replace(content, "\r\n", "\n");
    break;
  case EOL_TYPE_WINDOWS:
    // keep window newlines
    // remove unix newline in the document.
    search_and_replace(content, "\n", "\r\n");
    search_and_replace(content, "\r\r\n", "\r\n");
    break;
  default:
    printf("Warning: unknown EOL type: %d\n", eol_type);
  };
}

inline void search_and_replace(std::string & content, const std::string & token, const std::string & value) {
  if (token.empty())
    return;

  size_t pos = content.find(token, 0);
  while (pos != std::string::npos) {
    content.replace(content.begin() + pos, content.begin() + pos + token.size(), value);

    // next tag
    pos = content.find(token, pos + value.size());
  }
}

inline std::string load_file(const std::string & path) {
  std::ifstream file(path, std::ios::binary | std::ios::ate);
  size_t size = (size_t)file.tellg();
  file.seekg(0, std::ios::beg);

  std::string buffer;
  buffer.resize(size);
  if (buffer.capacity() < size)
    return std::string();

  if (file.read((char*)buffer.data(), size)) {
    return buffer;
  }
  return std::string();
}

inline bool save_file(const std::string & path, const std::string & content) {
  std::ofstream file(path, std::ios::binary | std::ios::ate);
  if (file.write(content.c_str(), content.size())) {
    return true;
  }
  return false;
}

inline bool file_exists(const std::string & name) {
  if (FILE *file = fopen(name.c_str(), "rb")) {
    fclose(file);
    return true;
  } else {
    return false;
  }
}

bool find_html_tag_boundaries(const std::string & content, const std::string & tag_name, size_t offset, HTML_TAG_INFO & info) {
  const std::string pattern_open  = "<" + tag_name;
  const std::string pattern_close = "</" + tag_name + ">";

  memset(&info, 0xff, sizeof(info));

  // Search the offsets of the <open ...> tag.
  size_t open_start = content.find(pattern_open, offset);
  if (open_start == std::string::npos)
    return false;
  size_t open_end = content.find(">", open_start);
  if (open_end == std::string::npos)
    return false;

  // Search the offsets of the </closing> tag.
  size_t close_start = content.find(pattern_close, open_end + 1);
  if (close_start == std::string::npos)
    return false;
  size_t close_end = close_start + pattern_close.size() - 1;
  if (close_end == std::string::npos)
    return false;

  info.open_start = open_start;
  info.open_end = open_end;
  info.close_start = close_start;
  info.close_end = close_end;

  info.inner_start = info.open_end    + 1;
  info.inner_end   = info.close_start - 1;
  //std::string::size_type inner_length = info.close_start - inner_start;
  //std::string inner_text = content.substr(inner_start, inner_length);

  return true;
}

bool find_html_attribute_boundaries(const std::string & content, const std::string & attr_name, size_t offset_start, size_t offset_end, HTML_ATTRIBUTE_INFO & info) {
  if (content.empty() || attr_name.empty() || offset_start >= offset_end)
    return false;

  memset(&info, 0xff, sizeof(info));

  // search for the pattern "id="
  std::string pattern_start = attr_name + "=";
  size_t pattern_pos = content.find(pattern_start, offset_start);
  if (pattern_pos == std::string::npos || pattern_pos >= offset_end)
    return false; // not found

  // look at the next character
  size_t next_pos = pattern_pos + pattern_start.size();
  if (next_pos == std::string::npos || next_pos >= offset_end)
    return false; // not found
  char next = content[next_pos];

  if (next == '\'' || next == '\"') {
    // this is a string enclosed argument
    // search for the string endding
    size_t end_pos = content.find(next, next_pos+1);
    if (end_pos == std::string::npos || end_pos >= offset_end)
      return false; // failed finding the attribute endding.

    info.attr_value_start = next_pos + 1;
    info.attr_value_end = end_pos - 1;
    return true;
  } else {
    // this is a word or numeric argument without quotes
    // search for the argument end

    size_t end_pos = content.find_first_of(html_tag_endding_characters, next);
    if (end_pos == std::string::npos || end_pos >= offset_end)
      return false; // failed finding the attribute endding.

    info.attr_value_start = next_pos;
    info.attr_value_end = end_pos - 1;
    return true;
  }

  return false;
}

inline std::string get_html_attribute_value(const std::string & content, const std::string & attr_name, const HTML_TAG_INFO & tag_info) {
  HTML_ATTRIBUTE_INFO attr_info;
  bool has_attr_value = find_html_attribute_boundaries(content, attr_name, tag_info.open_start, tag_info.open_end, attr_info);
  std::string attr_value;
  if (has_attr_value) {
    size_t attr_value_length = attr_info.attr_value_end - attr_info.attr_value_start + 1;
    attr_value = content.substr(attr_info.attr_value_start, attr_value_length);
  }
  return attr_value;
}

std::string find_class_property(const std::string & class_value, const std::string & property_name) {
  std::string pattern = property_name + ":";

  size_t pos = class_value.find(pattern);
  if (pos == std::string::npos)
    return std::string();

  std::string property_value;
  for(size_t i=pos+pattern.size(); i<class_value.size(); i++) {
    char c = class_value[i];
    if (c == ' ' || c == ';' || c == '\"' || c == '\0')
      break;
    property_value.append(1, c);
  }
  return property_value;
}

inline size_t get_html_tag_indentation(const char * content, const HTML_TAG_INFO & tag_info) {
  size_t indent = get_line_indentation(content, tag_info.open_start);
  return indent;
}

inline bool is_any(char c, const std::vector<char> & characters) {
  for (size_t i = 0; i < characters.size(); i++) {
    if (characters[i] == c)
      return true;
  }
  return false;
}

inline bool is_any(char c, const char * characters, size_t num_characters) {
  for (size_t i = 0; i < num_characters; i++) {
    if (characters[i] == c)
      return true;
  }
  return false;
}

inline std::string read_until(const std::string & content, size_t offset, const std::vector<char> & stop_characters) {
  std::string output;
  while (content[offset] != '\0') {
    if (is_any(content[offset], stop_characters)) {
      output.append(1, content[offset]);
      offset++;
    }
    else
      break;
  }
  return output;
}

inline std::string read_until(const std::string & content, size_t offset, const char * characters, size_t num_characters) {
  std::string output;
  while (content[offset] != '\0') {
    char c = content[offset];
    if (!is_any(c, characters, num_characters)) {
      output.append(1, c);
      offset++;
    }
    else
      break;
  }
  return output;
}

inline std::string read_html_tag(const std::string & content, size_t offset) {
  std::string output;
  output = read_until(content, offset, html_tag_endding_characters, num_html_tag_endding_characters);
  return output;
}

inline bool has_inner_html_tags(const std::string & content, int ignore_html_tag_flags) {
  size_t pos = content.find('<', 0);
  while (pos != std::string::npos) {

    // ignore html comments if required
    if ((ignore_html_tag_flags & HTML_TAG_COMMENT) == HTML_TAG_COMMENT) {
      static const std::string html_comment_open = "<!--";
      if (content.find(html_comment_open.c_str(), pos, html_comment_open.size()) == pos) {
        // that's an html comment
        pos = content.find('<', pos + html_comment_open.size());
        continue;
      }
    }

    // read the tag name
    std::string tag_name = read_html_tag(content, pos + 1);

    if (tag_name.empty()) {
      // we might have got a closing tag. ie </a>
      // next tag
      pos = content.find('<', pos + 1);
      continue;
    }

    if ((ignore_html_tag_flags & HTML_TAG_BR) == HTML_TAG_BR) {
      // br should not be considered as inner html tag and should be left alone.
      if (tag_name == "br") {
        // next tag
        pos = content.find('<', pos + tag_name.size());
        continue;
      }
    }
    if ((ignore_html_tag_flags & HTML_TAG_SUB) == HTML_TAG_SUB) {
      if (tag_name == "sub") {
        // next tag
        pos = content.find('<', pos + tag_name.size());
        continue;
      }
    }
    if ((ignore_html_tag_flags & HTML_TAG_SUP) == HTML_TAG_SUP) {
      if (tag_name == "sup") {
        // next tag
        pos = content.find('<', pos + tag_name.size());
        continue;
      }
    }

    // found one
    return true;
  }

  return false;
}

inline bool has_inner_html_tags(const HtmlTableRow & row, int ignore_html_tag_flags) {
  for(size_t i=0; i<row.size(); i++) {
    const HtmlTableCell & cell = row[i];
    if (has_inner_html_tags(cell, ignore_html_tag_flags))
      return true;
  }
  return false;
}

inline bool has_inner_html_tags(const HtmlTable & table, int ignore_html_tag_flags) {
  for(size_t i=0; i<table.rows.size(); i++) {
    const HtmlTableRow & row = table.rows[i];
    if (has_inner_html_tags(row, ignore_html_tag_flags))
      return true;
  }
  return false;
}

inline bool has_cell_spanning(const std::string & content) {
  if (content.find("rowspan") != std::string::npos) {
    return true;
  } else if (content.find("colspan") != std::string::npos) {
    return true;
  }
  return false;
}

inline std::vector<std::string> split(const std::string & content, char split_character) {
  std::stringstream input(content);
  std::string tmp;
  std::vector<std::string> elements;

  while(std::getline(input, tmp, split_character))
  {
     elements.push_back(tmp);
  }

  return elements;
}

inline std::string join(const std::vector<std::string> & lines, char join_character) {
  std::string output;
  for(size_t i=0; i<lines.size(); i++) {
    const std::string & line = lines[i];
    if (i > 0)
      output.append(1, join_character);
    output.append(line);
  }
  return output;
}

inline size_t trim(std::string & text, char trim_character) {
  size_t size_old = text.size();

  while(!text.empty() && text[0] == trim_character) {
    text.erase(0, 1);
  }

  while(!text.empty() && text[text.size()-1] == trim_character) {
    text.erase(text.size()-1, 1);
  }

  size_t size_new = text.size();
  size_t size_diff = size_old-size_new;
  return size_diff;
}

inline size_t trim(std::string & text, const char * trim_str) {
  if (trim_str == NULL)
    return 0;

  size_t size_old = text.size();

  size_t trim_str_size = strlen(trim_str);
  while(!text.empty() && text.find(trim_str, 0, trim_str_size) == 0) {
    text.erase(0, trim_str_size);
  }

  if (text.size() > trim_str_size) {
    size_t end_offset = text.size()-trim_str_size;
    while(!text.empty() && text.find(trim_str, end_offset) == end_offset) {
      text.erase(end_offset, trim_str_size);
    }
  }

  size_t size_new = text.size();
  size_t size_diff = size_old-size_new;
  return size_diff;
}

inline size_t trim_html_whitespace(std::string & text) {
  size_t count = 0;
  size_t diff = 0;
  do {
    diff = 0;  
    diff += trim(text, "\xc2\xa0");
    diff += trim(text, "&nbsp;");
    diff += trim(text, ' ');
    diff += trim(text, '\t');
    diff += trim(text, "\r\n");
    diff += trim(text, '\n');
    count += diff;
  } while(diff != 0);
  return count;
}

inline size_t trim_html_whitespace(HtmlTable & table) {
  size_t count = 0;
  for(size_t i=0; i<table.rows.size(); i++) {
    HtmlTableRow & row = table.rows[i];
    for(size_t j=0; j<row.size(); j++) {
      HtmlTableCell & cell = row[j];
      count += trim_html_whitespace(cell);
    }
  }
  return count;
}

inline bool is_html_white_character(const std::string & content, size_t offset) {
  static const std::string nbsp = "&nbsp;";
  if (offset < content.size()) {
    char c0 = content[offset + 0];
    char c1 = content[offset + 1];
    if (c0 == ' ' || c0 == '\t' || c0 == '\n')
      return true;
    else if (c0 == '\xc2' && c1 == '\xa0')
      return true;
    else if (c0 == '\r' && c1 == '\n')
      return true;
    else if (content.find(nbsp.c_str(), offset, nbsp.size()) == offset)
      return true;
  }
  return false;
}

inline size_t get_first_nonwhite_html_character(const char * content, size_t offset) {
  bool is_white = true;
  while(content[offset] != '\0' && is_white) {
    // check at this location
    is_white = is_html_white_character(content, offset);
    if (is_white)
      offset++;
  }
  if (!is_white)
    return offset;
  return std::string::npos;
}

inline void decrease_indent(std::vector<std::string> & lines, size_t num_spaces) {
  std::string tab_pattern(2, ' ');

  // Check that all given lines are indented
  for(size_t i=0; i<lines.size(); i++) {
    std::string & line = lines[i];
    bool line_indented = (line.size() >= num_spaces && line.substr(0, num_spaces) == tab_pattern);
    if (!line_indented)
      return; // all lines must be indented to decrease indentation
  }

  for(size_t i=0; i<lines.size(); i++) {
    std::string & line = lines[i];
    line.erase(0, num_spaces);
  }
}

inline std::string get_line_at_offset(std::string & content, size_t offset) {
  // Find the beginning of the line
  size_t offset_begin = offset;
  while(offset_begin > 0 && content[offset_begin - 1] != '\n')
    offset_begin--;

  // Find the end of the line
  size_t offset_end = offset;
  while(offset_end < content.size() - 1 && content[offset_end + 1] != '\n')
    offset_end++;

  if (offset_begin > offset_end)
    return std::string(); // Failed

  size_t count = offset_end - offset_begin + 1;
  std::string line = content.substr(offset_begin, count);
  return line;
}

/// <summary>
/// This filter is required since all my headers (h1, h2, h3...) are wrapped in a "<span>".
/// </summary>
/// <remarks>
/// For example: "# <span id="Introduction">Introduction</span>"
/// This filter removes the "<span>" tags but only if there is no html inside the tag.
/// </remarks>
void filter_span(std::string & content) {
  HTML_TAG_INFO info;
  size_t offset = 0;
  while(find_html_tag_boundaries(content, "span", offset, info)) {
    size_t tag_length = info.close_end - info.open_start + 1;
    std::string tag_content = content.substr(info.open_start, tag_length);
    size_t inner_length = info.close_start - info.inner_start;
    std::string inner_text = content.substr(info.inner_start, inner_length);

    bool has_inner_html = has_inner_html_tags(inner_text);
    if (has_inner_html) {
      // The inner text of the tag has more html inside. Do not proceed with the replacement. Do more passes to replace all the code.
      // next tag. Do not use close_end to prevent skipping tags within other tags of the same type.
      // For example, <span><span>foo bar</span></span>
      offset = info.open_end + 1;
    } else {
      // replace
      content.replace(content.begin() + info.open_start, content.begin() + info.close_end + 1, inner_text);
    }
  }
}

inline bool is_custom_css_class(const std::string & class_) {
  if (class_.find("pleasenote") != std::string::npos)
    return true;
  if (class_.find("postedit") != std::string::npos)
    return true;
  if (class_.find("renamep") != std::string::npos)
    return true;
  return false;
}

/// <summary>
/// This filter removes all <p> tags.
/// I use multiple custom css classes in my original wordpress posts. For example: 
///  * `pleasenote` is used to emphasise a note.
///  * `postedit`, to highlight post edits after publishing (after first public release).
/// This filter removes the "<p>" tags but only if there is no html inside the tag.
/// The "<p>" tag is skipped if it uses one of my custom css classes.
/// </summary>
void filter_paragraph(std::string & content) {
  HTML_TAG_INFO info;
  size_t offset = 0;
  while(find_html_tag_boundaries(content, "p", offset, info)) {
    size_t inner_length = info.close_start - info.inner_start;
    std::string inner_text = content.substr(info.inner_start, inner_length);

    std::string class_value = get_html_attribute_value(content, "class", info);
    bool has_custom_css_class = is_custom_css_class(class_value);

    bool has_inner_html = has_inner_html_tags(inner_text);
    if (has_inner_html) {
      // The inner text of the tag has more html inside. Do not proceed with the replacement. Do more passes to replace all the code.
      // next tag
      offset = info.close_end + 1;
    } else if (has_custom_css_class) {
      // Do not replace <p> tags that has our custom css classes.
      // next tag
      offset = info.close_end + 1;
    } else {

      // Clean up inner text
      trim_html_whitespace(inner_text);

      // replace
      content.replace(content.begin() + info.open_start, content.begin() + info.close_end + 1, inner_text);
    }
  }
}

/// <summary>
/// This filter replaces all <img> tags with their markdown equivalent, but only if there is no html inside the tag.
/// </summary>
void filter_images(std::string & content) {
  HTML_TAG_INFO info = {0};
  info.open_start = content.find("<img", 0);
  while(!content.empty() && info.open_start != std::string::npos) {
    size_t open_end_1 = content.find(">", info.open_start);
    size_t open_end_2 = content.find("/>", info.open_start);
    size_t open_end_pattern_length = 0;
    if (open_end_1 != std::string::npos && open_end_2 != std::string::npos)
      info.open_end = std::min(open_end_1, open_end_2);
    else if (open_end_1 != std::string::npos)
      info.open_end = open_end_1;
    else if (open_end_2 != std::string::npos)
      info.open_end = open_end_2;
    if (info.open_end == open_end_1)
      open_end_pattern_length = 1;
    else if (info.open_end == open_end_2)
      open_end_pattern_length = 2;

    std::string src = get_html_attribute_value(content, "src", info);
    std::string alt = get_html_attribute_value(content, "alt", info);

    if (!src.empty() && !alt.empty()) {
      std::string markdown_image = std::string("![") + alt + "](" + src + ")";

      // replace
      content.replace(content.begin() + info.open_start, content.begin() + info.open_end + open_end_pattern_length, markdown_image);
    } else if (!src.empty()) {
      std::string markdown_image = std::string("![](") + src + ")";

      // replace
      content.replace(content.begin() + info.open_start, content.begin() + info.open_end + open_end_pattern_length, markdown_image);

      // next tag
      info.open_start = content.find("<img", info.open_start);
    } else {
      // next tag
      info.open_start = content.find("<img", info.open_start + 1);
    }
  }
}

/// <summary>
/// This filter replaces all <a> tags with their markdown equivalent, but only if there is no html inside the tag.
/// </summary>
void filter_anchors(std::string & content) {
  HTML_TAG_INFO info;
  size_t offset = 0;
  while(find_html_tag_boundaries(content, "a", offset, info)) {
    size_t inner_length = info.close_start - info.inner_start;
    std::string inner_text = content.substr(info.inner_start, inner_length);

    std::string href_value  = get_html_attribute_value(content, "href", info);
    std::string title_value = get_html_attribute_value(content, "title", info);

    bool has_inner_html = has_inner_html_tags(inner_text);
    if (has_inner_html || href_value.empty() || inner_text.empty()) {
      // The inner text of the tag has more html inside. Do not proceed with the replacement. Do more passes to replace all the code.
      // next tag
      offset = info.close_end + 1;
    } else {
      std::string markdown_anchor;
      
      if (!inner_text.empty()) {
        markdown_anchor = std::string("[") + inner_text + "](" + href_value + ")";
        if (!title_value.empty()) {
          markdown_anchor.erase(markdown_anchor.size() - 1, 1);
          markdown_anchor.append(" \"");
          markdown_anchor.append(title_value);
          markdown_anchor.append("\")");
        }
      }

      // replace
      content.replace(content.begin() + info.open_start, content.begin() + info.close_end + 1, markdown_anchor);
    }
  }
}

/// <summary>
/// This filter replaces all <strong> tags with their markdown equivalent, but only if there is no html inside the tag.
/// </summary>
void filter_strong(std::string & content, const char * tag_name) {
  HTML_TAG_INFO info;
  size_t offset = 0;
  while(find_html_tag_boundaries(content, tag_name, offset, info)) {
    size_t inner_length = info.close_start - info.inner_start;
    std::string inner_text = content.substr(info.inner_start, inner_length);

    bool has_inner_html = has_inner_html_tags(inner_text);
    if (has_inner_html) {
      // The inner text of the tag has more html inside. Do not proceed with the replacement. Do more passes to replace all the code.
      // next tag
      offset = info.close_end + 1;
    } else {
      std::string markdown_strong;
      
      if (!inner_text.empty()) {
        markdown_strong = std::string("**") + inner_text + "**";
      }

      // replace
      content.replace(content.begin() + info.open_start, content.begin() + info.close_end + 1, markdown_strong);
    }
  }
}

/// <summary>
/// This filter replaces all <strong> tags with their markdown equivalent, but only if there is no html inside the tag.
/// </summary>
inline void filter_strong(std::string & content) {
  filter_strong(content, "strong");
  filter_strong(content, "b");
}

/// <summary>
/// This filter replaces all <i> tags with their markdown equivalent, but only if there is no html inside the tag.
/// </summary>
void filter_italic(std::string & content) {
  HTML_TAG_INFO info;
  size_t offset = 0;
  while(find_html_tag_boundaries(content, "i", offset, info)) {
    size_t inner_length = info.close_start - info.inner_start;
    std::string inner_text = content.substr(info.inner_start, inner_length);

    bool has_inner_html = has_inner_html_tags(inner_text);
    if (has_inner_html) {
      // The inner text of the tag has more html inside. Do not proceed with the replacement. Do more passes to replace all the code.
      // next tag
      offset = info.close_end + 1;
    } else {
      std::string markdown_italic;
      
      if (!markdown_italic.empty()) {
        markdown_italic = std::string("_") + inner_text + "_";
      }

      // replace
      content.replace(content.begin() + info.open_start, content.begin() + info.close_end + 1, markdown_italic);
    }
  }
}

/// <summary>
/// This filter replaces all <em> tags with their markdown equivalent, but only if there is no html inside the tag.
/// </summary>
void filter_emphasized(std::string & content) {
  HTML_TAG_INFO info;
  size_t offset = 0;
  while(find_html_tag_boundaries(content, "em", offset, info)) {
    size_t inner_length = info.close_start - info.inner_start;
    std::string inner_text = content.substr(info.inner_start, inner_length);

    bool has_inner_html = has_inner_html_tags(inner_text);
    if (has_inner_html) {
      // The inner text of the tag has more html inside. Do not proceed with the replacement. Do more passes to replace all the code.
      // next tag
      offset = info.close_end + 1;
    } else {
      std::string markdown_emphasized;
      
      if (!markdown_emphasized.empty()) {
        markdown_emphasized = std::string("_") + inner_text + "_";
      }

      // replace
      content.replace(content.begin() + info.open_start, content.begin() + info.close_end + 1, markdown_emphasized);
    }
  }
}

/// <summary>
/// This filter replaces all <code> tags with their markdown equivalent, but only if there is no html inside the tag.
/// </summary>
/// <remarks>
/// If the text inside the tag is on a single line,  the content is wrapped inside '`' (backticks characters).
/// If the text inside the tag is on multiple lines, the content is wrapped inside fenced code blocks (a triple "```" character sequence)
/// </remarks>
void filter_code(std::string & content) {
  HTML_TAG_INFO info;
  size_t offset = 0;
  while(find_html_tag_boundaries(content, "code", offset, info)) {
    size_t inner_length = info.close_start - info.inner_start;
    std::string inner_text = content.substr(info.inner_start, inner_length);

    bool has_inner_html = has_inner_html_tags(inner_text);
    if (has_inner_html) {
      // The inner text of the tag has more html inside. Do not proceed with the replacement. Do more passes to replace all the code.
      // next tag
      offset = info.close_end + 1;
    } else {

      bool is_single_line = (inner_text.find('\n') == std::string::npos);

      trim_html_whitespace(inner_text);

      std::string markdown_code;
      if (!inner_text.empty()) {
        if (is_single_line) {
          markdown_code = std::string("`") + inner_text + "`";
        } else {
          markdown_code = std::string("\n```\n") + inner_text + "\n```\n";
        }
      }

      // replace
      content.replace(content.begin() + info.open_start, content.begin() + info.close_end + 1, markdown_code);
    }
  }
}

/// <summary>
/// This filter replaces all <li> tags with their markdown equivalent, but only if there is no html inside the tag.
/// </summary>
void filter_list_item(std::string & content) {
  HTML_TAG_INFO info;
  size_t offset = 0;
  while(find_html_tag_boundaries(content, "li", offset, info)) {
    size_t inner_length = info.close_start - info.inner_start;
    std::string inner_text = content.substr(info.inner_start, inner_length);

    bool has_inner_html = has_inner_html_tags(inner_text);
    if (has_inner_html) {
      // The inner text of the tag has more html inside. Do not proceed with the replacement. Do more passes to replace all the code.
      // next tag
      offset = info.close_end + 1;
    } else {
      trim_html_whitespace(inner_text);
      std::string markdown_list_item = std::string("* ") + inner_text;

      // replace
      content.replace(content.begin() + info.open_start, content.begin() + info.close_end + 1, markdown_list_item);
    }
  }
}

/// <summary>
/// This filter replaces all <ul> tags with their markdown equivalent, but only if there is no html inside the tag.
/// </summary>
void filter_unordered_lists(std::string & content) {
  HTML_TAG_INFO info;
  size_t offset = 0;
  while(find_html_tag_boundaries(content, "ul", offset, info)) {
    size_t inner_length = info.close_start - info.inner_start;
    std::string inner_text = content.substr(info.inner_start, inner_length);

    trim_html_whitespace(inner_text);

    // replace
    content.replace(content.begin() + info.open_start, content.begin() + info.close_end + 1, inner_text);
  }
}

/// <summary>
/// This filter replaces all css justified <li> tags by plain <li> tags.
/// This filter must be run before `filter_list_item()` function is called.
/// </summary>
/// <remarks>
/// This is a custom issue about my posts on my previous web site. Some lists was formatted justified.
/// This filter basically runs a search and replace to replace erase the "style='text-align: justify;'" html attribute in list items.
/// </remarks>
void filter_list_item_simplify(std::string & content) {
  static const std::string complex_list_item = "<li style=\"text-align: justify;\">";
  static const std::string simplified_list_item = "<li>";
  size_t offset = content.find(complex_list_item);
  while(offset != std::string::npos) {
    // replace by a simplified version of the justified list item
    content.replace(content.begin() + offset, content.begin() + offset + complex_list_item.size(), simplified_list_item);

    // next tag
    offset = content.find(complex_list_item, offset);
  }
}

/// <summary>
/// This filter removes all wordpress-generated <pre> tags which directly encloses <code> tags.
/// </summary>
/// <remarks>
/// This filter must be run before `filter_code()` function is called.
/// </remarks>
void filter_preformatted_code_simplify(std::string & content) {
  static const std::string complex_preformatted = "<pre class=\"wp-block-code\"><code>";
  static const std::string simplified_preformatted = "<code>";
  static const std::string complex_close_pattern = "</code></pre>";
  static const std::string simplified_close_pattern = "</code>";
  size_t offset = content.find(complex_preformatted);
  while(offset != std::string::npos) {
    // replace by a simplified version of the preformatted code
    content.replace(content.begin() + offset, content.begin() + complex_preformatted.size() + 1, simplified_preformatted);

    // search for the closing tags
    size_t offset_close = content.find(complex_close_pattern);
    if (offset_close != std::string::npos) {
      // replace by a simplified version of the preformatted code
      content.replace(content.begin() + offset_close, content.begin() + complex_close_pattern.size() + 1, simplified_close_pattern);
    }

    // next tag
    offset = content.find(complex_preformatted, offset);
  }
}

/// <summary>
/// This filter removes all <div> tags, but only if there is no html inside the tag.
/// </summary>
/// <remarks>
/// Most <div> tags are file downloads (attachments) like `<div id="attachment_2357" style="width: 560px" class="wp-caption alignnone">`.
/// Divisions that are galleries must be processed first with function `filter_division_gallery()`.
/// </remarks>
void filter_division(std::string & content) {
  HTML_TAG_INFO info;
  size_t offset = 0;
  while(find_html_tag_boundaries(content, "div", offset, info)) {
    size_t inner_length = info.close_start - info.inner_start;
    std::string inner_text = content.substr(info.inner_start, inner_length);

    std::string class_value = get_html_attribute_value(content, "class", info);
    bool has_custom_css_class = is_custom_css_class(class_value);

    bool has_inner_html = has_inner_html_tags(inner_text);
    if (has_inner_html) {
      // The inner text of the tag has more html inside. Do not proceed with the replacement. Do more passes to replace all the code.
      // next tag
      offset = info.close_end + 1;
    } else if (has_custom_css_class) {
      // Do not replace <div> tags that has our custom css classes.
      // next tag
      offset = info.close_end + 1;
    } else {
      
      // Check if the indentation of the content inside the division must be decreased.
      size_t div_indentation = 0;
      size_t inner_html_indentation = 0;
      if (content[info.inner_start] == '\n' || (content[info.inner_start] == '\r' && content[info.inner_start+1] == '\n')) { // If div is the only tag on the line
        size_t div_html_length = info.close_end - info.open_start;
        std::string div_html = content.substr(info.open_start, div_html_length); 

        // Check if the inner_html is more indented than the divivision itself
        div_indentation = get_line_indentation(content.c_str(), info.open_start);
        size_t nonwhite_pos = get_first_nonwhite_html_character(content.c_str(), info.inner_start);
        if (nonwhite_pos != std::string::npos) {
          inner_html_indentation = get_line_indentation(content.c_str(), nonwhite_pos);
        }
      }

      // Remove first and last newline characters
      if (!inner_text.empty() && inner_text[0] == '\n')
        inner_text.erase(0, 1);
      if (!inner_text.empty() && inner_text[inner_text.size()-1] == '\n')
        inner_text.erase(inner_text.size()-1, 1);

      // Check if we need to decease the indentation of the inner_text
      if (inner_html_indentation > div_indentation) {

        // Decrease indentation of inner_text
        std::vector<std::string> lines = split(inner_text, '\n');
        decrease_indent(lines, 2);
        inner_text = join(lines, '\n');

        // and then trim the inner text
        trim_html_whitespace(inner_text);
      } else {
        // Trim the inner text
        trim_html_whitespace(inner_text);
      }

      // replace
      content.replace(content.begin() + info.open_start, content.begin() + info.close_end + 1, inner_text);
    }
  }
}

inline bool is_division_gallery(const std::string & content, const HTML_TAG_INFO & info) {
  if (content.empty() ||
    (info.open_start == 0 && info.open_end == 0) ||
    (info.open_start == std::string::npos || info.open_end == std::string::npos))
    return false;

  std::string id_value = get_html_attribute_value(content, "id", info);
  std::string class_value = get_html_attribute_value(content, "id", info);

  if (id_value.find("gallery") != std::string::npos && class_value.find("gallery") != std::string::npos)
    return true;
  return false;
}

bool parse_gallery_table(std::string & content, HtmlTable & table) {
  table.first_row_is_header = false;

  HtmlTableRow tmp_row;

  HTML_TAG_INFO dl_info;
  size_t dl_offset = 0;
  while (find_html_tag_boundaries(content, "dl", dl_offset, dl_info)) {
    size_t dl_inner_length = dl_info.close_start - dl_info.inner_start;
    std::string dl_inner_text = content.substr(dl_info.inner_start, dl_inner_length);

    HTML_TAG_INFO dt_info;
    size_t dt_offset = 0;
    while (find_html_tag_boundaries(dl_inner_text, "dt", dt_offset, dt_info)) {
      size_t dt_inner_length = dt_info.close_start - dt_info.inner_start;
      std::string dt_inner_text = dl_inner_text.substr(dt_info.inner_start, dt_inner_length);

      // Create a new cell in current row
      tmp_row.push_back(dt_inner_text);

      // next tag dt
      dt_offset = dt_info.close_end + 1;
    }

    // If we read all column inside the row, flush the temporary row
    if (!tmp_row.empty()) {
      // Flush the row accumulator to the table
      table.rows.push_back(tmp_row);
      tmp_row.clear();
    }

    // next tag dl
    dl_offset = dl_info.close_end + 1;
  }

  // If there is still cells in the temporary row (accumulator), flush them in the table
  if (!tmp_row.empty()) {
    table.rows.push_back(tmp_row);
    tmp_row.clear();
  }

  if (!table.rows.empty()) {
    return true;
  }
  return false;
}

/// <summary>
/// This filter removes all <div> tags, which are displaying image galleries, but only if there is no html inside the tag.
/// </summary>
/// <remarks>
/// For example:
///    <div id='gallery-22' class='gallery galleryid-1249 gallery-columns-3 gallery-size-thumbnail gallery1'>
///      <dl class="gallery-item">
///        <dt class="gallery-icon">
///          <a href="http://www.end2endzone.com/wp-content/uploads/2016/02/Multiplexing-Analog-Knob-with-1x-2-Position-and-1x-3-Position-switches-Data-Capture-Sample.png" title="Demultiplexing an analog knob with a 2-Position and a 3-Position switch : Sample data" rel="gallery1"><img src="http://www.end2endzone.com/wp-content/uploads/2016/02/Multiplexing-Analog-Knob-with-1x-2-Position-and-1x-3-Position-switches-Data-Capture-Sample-146x150.png" width="146" height="150" alt="Demultiplexing an analog knob with a 2-Position and a 3-Position switch : Sample data" /></a><span><a class="void" href="http://www.end2endzone.com/wp-content/uploads/2016/02/Multiplexing-Analog-Knob-with-1x-2-Position-and-1x-3-Position-switches-Data-Capture-Sample.png" rel="nolightbox" target="_blank">498x510</a></span>
///        </dt>
///        <dd class="gallery-caption" id="caption1519">
///          <span class="imagecaption">Demultiplexing an analog knob with a 2-Position and a 3-Position switch : Sample data</span><br /> <span class="imagedescription">Demultiplexing an analog knob with a 2-Position and a 3-Position switch : Sample data</span><br />
///        </dd>
///      </dl>
///      <br style='clear: both' />
///    </div>
/// </remarks>
void filter_division_gallery(std::string & content) {
  HTML_TAG_INFO info;
  size_t offset = 0;
  while(find_html_tag_boundaries(content, "div", offset, info)) {
    size_t inner_length = info.close_start - info.inner_start;
    std::string inner_text = content.substr(info.inner_start, inner_length);

    if (!is_division_gallery(content, info)) {
      // The inner text of the tag has more html inside. Do not proceed with the replacement. Do more passes to replace all the code.
      // next tag
      offset = info.close_end + 1;
    } else {

      // Parse the gallery
      HtmlTable table;
      table.first_row_is_header = false;
      bool parsed = parse_gallery_table(inner_text, table);

      if (parsed && !has_inner_html_tags(table)) {
        // trim the table
        trim_html_whitespace(table);

        // convert to markdown
        std::string markdown_table = to_markdown(table);

        // replace
        content.replace(content.begin() + info.open_start, content.begin() + info.close_end + 1, markdown_table);
      } else {
        // next tag
        offset = info.close_end + 1;
      }
    }
  }
}

bool parse_html_table(std::string & content, HtmlTable & table) {
  table.first_row_is_header = false;

  HtmlTableRow tmp_row;

  HTML_TAG_INFO tr_info;
  size_t tr_offset = 0;
  while (find_html_tag_boundaries(content, "tr", tr_offset, tr_info)) {
    size_t tr_inner_length = tr_info.close_start - tr_info.inner_start;
    std::string tr_inner_text = content.substr(tr_info.inner_start, tr_inner_length);

    // Check if this row is a header
    bool is_first_row = (table.rows.size() == 0);
    std::string class_value = get_html_attribute_value(content, "class", tr_info);
    if (class_value.find("header") != std::string::npos) {
      table.first_row_is_header = true;
    }

    HTML_TAG_INFO td_info;
    size_t td_offset = 0;
    while (find_html_tag_boundaries(tr_inner_text, "td", td_offset, td_info)) {
      size_t td_inner_length = td_info.close_start - td_info.inner_start;
      std::string td_inner_text = tr_inner_text.substr(td_info.inner_start, td_inner_length);

      std::string colspan_value = get_html_attribute_value(tr_inner_text, "colspan", td_info);

      // Create a new cell in current row
      tmp_row.push_back(td_inner_text);

      // Check if the spell spans across multiple column
      if (!colspan_value.empty() && is_numeric(colspan_value.c_str())) {
        // markdown does not support colspan, repeat the cell multiple times.
        int colspan = -1;
        parse_value(colspan_value, colspan);
        if (colspan > 1) {
          for(int i=1; i<colspan; i++) {
            tmp_row.push_back("");
          }
        }
      }

      // next tag dt
      td_offset = td_info.close_end + 1;
    }

    // If we read all column inside the row, flush the temporary row
    if (!tmp_row.empty()) {
      // Flush the row accumulator to the table
      table.rows.push_back(tmp_row);
      tmp_row.clear();
    }

    // next tag dl
    tr_offset = tr_info.close_end + 1;
  }

  // If there is still cells in the temporary row (accumulator), flush them in the table
  if (!tmp_row.empty()) {
    table.rows.push_back(tmp_row);
    tmp_row.clear();
  }

  if (!table.rows.empty()) {
    return true;
  }
  return false;
}

/// <summary>
/// This filter replaces all <table> tags with their markdown equivalent, but only if there is no html inside the tag.
/// Unless the table header is obviously identified with css of with tags, an empty header is inserted since markdown does not support tables without headers.
/// </summary>
void filter_table(std::string & content) {
  HTML_TAG_INFO info;
  size_t offset = 0;
  while(find_html_tag_boundaries(content, "table", offset, info)) {
    size_t inner_length = info.close_start - info.inner_start;
    std::string inner_text = content.substr(info.inner_start, inner_length);

    // Skip html tables that have colspan or rowspan cells which is not supported in markdown
    if (!has_cell_spanning(inner_text)) {

      // Parse the table
      HtmlTable table;
      table.first_row_is_header = false;
      bool parsed = parse_html_table(inner_text, table);

      if (parsed && !has_inner_html_tags(table)) {
        // trim the table
        trim_html_whitespace(table);

        // convert to markdown
        std::string markdown_table = to_markdown(table);

        // replace
        content.replace(content.begin() + info.open_start, content.begin() + info.close_end + 1, markdown_table);
      } else {
        // next tag
        offset = info.close_end + 1;
      }
    } else {
      // next tag
      offset = info.close_end + 1;
    }
  }
}

/// <summary>
/// This filter replaces all <pre> tags with their markdown equivalent, but only if there is no html inside the tag.
/// Most <pre> tags are used for embedding code.
/// </summary>
/// <remarks>
/// See also `filter_code()` for details.
/// The `Crayon Syntax Highlighter` wordpress plugin also generates <div> and <pre> tags. For example:
///    <div class="crayon-line">
///      <pre class="lang:c++ decode:true" title="Arduino tone and delay functions overrides" data-url="http://www.end2endzone.com/wp-content/uploads/2016/10/Arduino-tone-and-delay-functions-overrides.ino">http://www.end2endzone.com/wp-content/uploads/2016/10/Arduino-tone-and-delay-functions-overrides.ino</pre>
///    </div>
/// </remarks>
void filter_preformatted(std::string & content) {
  HTML_TAG_INFO info;
  size_t offset = 0;
  while(find_html_tag_boundaries(content, "pre", offset, info)) {
    size_t inner_length = info.close_start - info.inner_start;
    std::string inner_text = content.substr(info.inner_start, inner_length);

    std::string lang;

    HTML_ATTRIBUTE_INFO data_url_info;
    bool has_data_url_value = find_html_attribute_boundaries(content, "data-url", info.open_start, info.open_end, data_url_info);

    std::string class_value = get_html_attribute_value(content, "class", info);
    if (!class_value.empty()) {
      // get the 'lang' parameter
      lang = find_class_property(class_value, "lang");
      if (lang == "default")
        lang = "";
      else if (lang == "c++")
        lang = "cpp";
    }

    bool has_inner_html = has_inner_html_tags(inner_text);
    if (has_inner_html) {
      // The inner text of the tag has more html inside. Do not proceed with the replacement. Do more passes to replace all the code.
      // Search for the next tag
      offset = info.close_end + 1;
    } else {

      bool is_single_line = (inner_text.find('\n') == std::string::npos);
      if (has_data_url_value) {
        is_single_line = false;

        // Read the url of the file and trim the hostname from the url
        std::string data_url_value = get_html_attribute_value(content, "data-url", info);
        search_and_replace(data_url_value, WEBSITE_HOSTNAME, "");

        // Create hugo function to include the data-url file into this content
        inner_text.clear();
        inner_text.append("{{< readfile file=\"");
        inner_text.append(data_url_value);
        inner_text.append("\" >}}");
      }
      else
        trim_html_whitespace(inner_text);

      std::string markdown_code;
      if (is_single_line) {
        markdown_code = std::string("`") + inner_text + "`";
      } else {
        markdown_code = std::string("\n```") + lang + "\n" + inner_text + "\n```\n";
      }

      // replace
      content.replace(content.begin() + info.open_start, content.begin() + info.close_end + 1, markdown_code);
    }
  }
}

void filter_known_html_entities(std::string & content) {
  // look for &nbsp; encoded as a utf8 code point
  search_and_replace(content, "\xc2\xa0", "&nbsp;");

  search_and_replace(content, "&#215;", "x");
  search_and_replace(content, "&#8211;", "-");
  search_and_replace(content, "&#8217;", "'");
  search_and_replace(content, "&#8230;", "...");
  search_and_replace(content, "&#8220;", "&quot;");
  search_and_replace(content, "&#8221;", "&quot;");
  search_and_replace(content, "&#8242;", "'");
}

/// <summary>
/// This filter replaces all non-breaking space ( "&nbsp;" ) entities by a normal space, where a non-breaking space is not mandatory.
/// For examples, if the non-breaking space is between two words, a normal space is enough.
/// At multiple locations, wordpress seems to had prefered to insert "&nbsp;" instead of normal spaces.
/// </summary>
void filter_useless_nbsp_entities(std::string & content) {
  static const std::string pattern = "&nbsp;";
  size_t pos = content.find(pattern);
  while(pos != std::string::npos) {

    // Check characters before and after.
    char c_before = '\0';
    char c_after = '\0';
    if (pos > 0)
      c_before = content[pos - 1];
    if (pos+1 < content.size())
      c_after = content[pos + 1];

    // If they are alphanumeric, the non-breaking space can be safely replaced by a normal space
    if (is_alphanumeric(c_before) && is_alphanumeric(c_after)) {
      // Replace the non-breaking space
      content.replace(content.begin() + pos, content.begin() + pos + pattern.size(), " ");

      // next nbsp entity
      pos = content.find(pattern, pos);
      continue;
    }
  
    // next nbsp entity
    pos = content.find(pattern, pos + 1);
  }
}

/// <summary>
/// This filter trims the content of all <td> tags, but only if there is no html inside the tag.
/// In other words, it removes all white space characters from the beginning and the end of the inner html.
/// </summary>
void filter_table_cells_inner_white_space(std::string & content) {
  HTML_TAG_INFO info;
  size_t offset = 0;
  while(find_html_tag_boundaries(content, "td", offset, info)) {
    size_t inner_length = info.close_start - info.inner_start;
    std::string inner_text = content.substr(info.inner_start, inner_length);

    if (!has_inner_html_tags(inner_text)) {
      // trim the cell on a single line
      trim_html_whitespace(inner_text);

      // replace the tag's inner text
      content.replace(content.begin() + info.inner_start, content.begin() + info.inner_end + 1, inner_text);

      // next tag. Can't use close_end index as usual since replacing the inner text has modified close_end.
      offset = info.open_end + 1;
    } else {
      // next tag
      offset = info.close_end + 1;
    }
  }
}

/// <summary>
/// This filter trims the content between <td> tags.
/// In other words, it removes empty lines between a </td> and the next <td>.
/// </summary>
void filter_table_cells_outer_white_space(std::string & content) {
  // For each possible indentations
  for(size_t indent = 0; indent <= 10; indent += 2) {
    
    // Build the old pattern
    std::string pattern_old;
    pattern_old.append("</td>\n");
    pattern_old.append(indent, ' ');
    pattern_old.append("\n");
    pattern_old.append(indent, ' ');
    pattern_old.append("<td");

    // Build the new pattern
    std::string pattern_new;
    pattern_new.append("</td>\n");
    pattern_new.append(indent, ' ');
    pattern_new.append("<td");

    // Search & replace
    search_and_replace(content, pattern_old, pattern_new);

    // Rebuild the old pattern assuming empty lines that separate each <td> element
    pattern_old.clear();
    pattern_old.append("</td>\n");
    pattern_old.append("\n");
    pattern_old.append(indent, ' ');
    pattern_old.append("<td");

    // Search & replace
    search_and_replace(content, pattern_old, pattern_new);
  }
}

/// <summary>
/// This filter trims the content between <tr> tags.
/// In other words, it removes empty lines between a </tr> and the next <tr>.
/// </summary>
void filter_table_rows_outer_white_space(std::string & content) {
  // For each possible indentations
  for(size_t indent = 0; indent <= 10; indent += 2) {
    
    // Build the old pattern
    std::string pattern_old;
    pattern_old.append("</tr>\n");
    pattern_old.append(indent, ' ');
    pattern_old.append("\n");
    pattern_old.append(indent, ' ');
    pattern_old.append("<tr");

    // Build the new pattern
    std::string pattern_new;
    pattern_new.append("</tr>\n");
    pattern_new.append(indent, ' ');
    pattern_new.append("<tr");

    // Search & replace
    search_and_replace(content, pattern_old, pattern_new);

    // Rebuild the old pattern assuming empty lines that separate each <tr> element
    pattern_old.clear();
    pattern_old.append("</tr>\n");
    pattern_old.append("\n");
    pattern_old.append(indent, ' ');
    pattern_old.append("<tr");

    // Search & replace
    search_and_replace(content, pattern_old, pattern_new);




    // Rebuild the old pattern assuming empty lines that separate each the last </td> tag or the row
    pattern_old.clear();
    pattern_old.append("</td>\n");
    pattern_old.append("\n");
    pattern_old.append(indent, ' ');
    pattern_old.append("</tr>");

    // Rebuild the new pattern as well
    pattern_new.clear();
    pattern_new.append("</td>\n");
    pattern_new.append(indent, ' ');
    pattern_new.append("</tr>");

    // Search & replace
    search_and_replace(content, pattern_old, pattern_new);

  }
}

/// <summary>
/// This filter looks for Reference-style links and converts them to inline links.
/// See https://www.markdownguide.org/basic-syntax/#reference-style-links and https://commonmark.org/help/tutorial/07-links.html for details.
/// </summary>
void force_inline_hyperlinks(std::string & content) {
  for(size_t link_reference_id = 1; link_reference_id < std::string::npos; link_reference_id++) {
    
    // Search for the link position in text
    std::string text_pattern = std::string("][") + to_string(link_reference_id) + "]";
    std::string reference_pattern = std::string("\n [") + to_string(link_reference_id) + "]: ";
    
    size_t reference_pos = content.find(reference_pattern);
    if (reference_pos == std::string::npos)
      break; // All references processed.

    // Extract url from reference
    std::string url;
    if (reference_pos != std::string::npos) {
      url = read_until(content, reference_pos + reference_pattern.size(), link_reference_endding_characters, num_link_reference_endding_characters);
      if (url.empty())
        break; // Failed to find url
    }

    // Remove the bottom reference definition
    content.replace(content.begin() + reference_pos, content.begin() + reference_pos + reference_pattern.size() + url.size(), "");

    // Replace all text links of this refererence in content
    size_t text_pos = content.find(text_pattern);
    while(text_pos != std::string::npos) {
      // replace text link
      std::string inline_link_content = std::string("](") + url + ")";
      content.replace(content.begin() + text_pos, content.begin() + text_pos + text_pattern.size(), inline_link_content);

      // next link reference
      text_pos = content.find(text_pattern, text_pos);
    }
  }
}

/// <summary>
/// This filter adds newline after some tags if text follows immediately after the tag close.
/// </summary>
/// <remarks>
/// For example:
///   <div id="attachment_1197" style="width: 404px" class="wp-caption alignnone">
///     <a href="http://www.end2endzone.com/wp-content/uploads/2015/11/VirtualDub-frameserver-mode.png"><img aria-describedby="caption-attachment-1197" loading="lazy" class="size-full wp-image-1197" src="http://www.end2endzone.com/wp-content/uploads/2015/11/VirtualDub-frameserver-mode.png" alt="VirtualDub Frameserver Mode" width="394" height="205" srcset="http://www.end2endzone.com/wp-content/uploads/2015/11/VirtualDub-frameserver-mode.png 394w, http://www.end2endzone.com/wp-content/uploads/2015/11/VirtualDub-frameserver-mode-150x78.png 150w, http://www.end2endzone.com/wp-content/uploads/2015/11/VirtualDub-frameserver-mode-300x156.png 300w" sizes="(max-width: 394px) 100vw, 394px" /></a>
///     <p id="caption-attachment-1197" class="wp-caption-text">
///       VirtualDub Frameserver Mode
///     </p>
///   </div> The frameserver is now ready to provide frames to other applications.
/// </remarks>
void filter_missing_newline(std::string & content, const std::string & tag_close_definition) {
  size_t tag_close_pos = content.find(tag_close_definition);
  while(tag_close_pos != std::string::npos) {
    // Check the character following
    char next = '\0';
    size_t next_offset = tag_close_pos + tag_close_definition.size();
    if (next_offset < content.size()) {
      next = content[next_offset];
    }

    if (next == '\0')
      return; // we reached the end of the document.

    // Should a newline must be inserted ?
    if (next != '\n') {

      // If the next character is a space, replace it.
      if (next == ' ')
        content[next_offset] = '\n';
      else {
        // Otherwise, insert a newline
        content.insert(next_offset, 1, '\n');
      }
    }

    // next tag
    tag_close_pos = content.find(tag_close_definition, tag_close_pos + 1);
  }
}

/// <summary>
/// This filter replaces all "featured_image:" elements in front matter by the new way of defining images.
/// </summary>
/// <remarks>
/// For examples:
///   featured_image: /wp-content/uploads/2000/01/foobar.png
/// Into:
///   images:
///     - src: /wp-content/uploads/2000/01/foobar.png
/// </remarks>
void filter_featured_image(std::string & content) {
  static const std::string pattern = "featured_image: ";
  static const std::string empty_string;

  size_t pos = content.find(pattern);
  while(pos != std::string::npos) {
    // read until the end of the line
    std::string line = get_line_at_offset(content, pos);

    // Get the url of the featured image
    std::string featured_image_url = line;
    search_and_replace(featured_image_url, pattern, empty_string);

    std::string yaml = "images:\n  - src: " + featured_image_url;

    //replace in document
    search_and_replace(content, line, yaml);

    // next
    pos = content.find(pattern, pos);
  }
}

/// <summary>
/// After some closing tags, a newline should be inserted if missing.
/// </summary>
void filter_missing_newline(std::string & content) {
  filter_missing_newline(content, "</div>");
  filter_missing_newline(content, "</table>");
  filter_missing_newline(content, "</p>");
}

/// <summary>
/// This filter adds newline after the end of an header.
/// </summary>
/// <remarks>
/// Headers with a missing newline are identified as the following:
/// ## <span id="Design">Design</span> The following section illustrate the design
/// </remarks>
void filter_missing_newline_after_header(std::string & content) {
  HTML_TAG_INFO info;
  size_t offset = 0;
  while(find_html_tag_boundaries(content, "span", offset, info)) {
    size_t inner_length = info.close_start - info.inner_start;
    std::string inner_text = content.substr(info.inner_start, inner_length);

    bool has_inner_html = has_inner_html_tags(inner_text);
    if (has_inner_html) {
      // The inner text of the tag has more html inside. Do not proceed with the replacement. Do more passes to replace all the code.
      // next tag
      offset = info.close_end + 1;
    } else {

      // Is this span a markdown header ?
      std::string line = get_line_at_offset(content, info.open_start);
      trim_html_whitespace(line);
      if (!line.empty() && line[0] == '#') {

        // Check the next character following </span>
        char next = '\0';
        size_t next_offset = info.close_end + 1;
        if (next_offset < content.size()) {
          next = content[next_offset];
        }

        if (next == '\0')
          return; // we reached the end of the document.

        // Should a newline must be inserted ?
        if (next != '\n') {

          // If the next character is a space, replace it.
          if (next == ' ')
            content[next_offset] = '\n';
          else {
            // Otherwise, insert a newline
            content.insert(next_offset, 1, '\n');
          }
        }
      }

      // next tag
      offset = info.close_end + 1;
    }
  }
}

void filter_comment_separators(std::string & content) {
  static const std::string pattern = "<!--=================================================-->";
  static const std::string empty_string;

  search_and_replace(content, std::string() + "\n" + pattern + "\n", empty_string);
  search_and_replace(content, std::string() + pattern + " ", empty_string);
  search_and_replace(content, std::string() + pattern, empty_string);
}

/// <summary>
/// This filter removes "type: post" inside the front matter.
/// Hugo does not seems to like this. Posts with "type: post" don't show up in "recent posts" sections.
/// </summary>
void filter_type_post(std::string & content) {
  static const std::string pattern = "type: post\n";
  static const std::string empty_string;

  search_and_replace(content, pattern, empty_string);
}

/// <summary>
/// This filter removes messed up "more" identifiers.
/// For an unknown reasons, some of my posts have a french-english "more" indentifier.
/// </summary>
void filter_more_comment(std::string & content) {
  static const std::string pattern = "<!--more Lire la suite →-->";
  static const std::string value = "<!--more-->";

  search_and_replace(content, pattern, value);
}

/// <summary>
/// Execute all filters one by one.
/// </summary>
void run_all_filters(std::string & content) {
  EOL_TYPE eol_type = normalize_newlines(content);
  filter_preformatted_code_simplify(content);
  filter_known_html_entities(content);
  filter_useless_nbsp_entities(content);
  filter_comment_separators(content);
  filter_missing_newline_after_header(content);
  filter_missing_newline(content);
  filter_type_post(content);
  filter_more_comment(content);
  filter_featured_image(content);

  // run multiple passes for tag that can embed other tags
  static const size_t num_passes = 15;
  for(size_t i=0; i<num_passes; i++) {
    filter_span(content);
    filter_paragraph(content);
    filter_images(content);
    filter_strong(content);
    filter_italic(content);
    filter_anchors(content);
    filter_emphasized(content);
    filter_code(content);
    filter_unordered_lists(content);
    filter_list_item_simplify(content);
    filter_list_item(content);
    filter_division_gallery(content);
    filter_table(content);
    filter_division(content);
    filter_preformatted(content);
  }

  filter_table_cells_inner_white_space(content);
  filter_table_cells_outer_white_space(content);
  filter_table_rows_outer_white_space(content);

  force_inline_hyperlinks(content);

  restore_newlines(content, eol_type);
}

int main(int argc, char* argv[])
{
  if (argc <= 1) {
    std::cout << "Error. Please specify an input file.\n";
    return 1;
  }

  const char * path = argv[1];
  if (!file_exists(path)) {
    std::cout << "File not found: '" << path << "'.\n";
    return 2;
  }

  std::cout << "Loading file '" << path << "'.\n";
  std::string content = load_file(path);
  if (content.empty()) {
    std::cout << "Error. Unable to load file '" << path << "'.\n";
    return 3;
  }

  run_all_filters(content);

  // use in-place replacement
  std::string output_path;
  if (process_file_in_place)
    output_path = std::string(path);
  else
    output_path = std::string(path)+".backup.md";

  // show output message
  if (process_file_in_place)
    std::cout << "Saving file.\n";
  else
    std::cout << "Saving file as '" << output_path << "'.\n";

  bool saved = save_file(output_path, content);
  if (!saved) {
    std::cout << "Error. Unable to save file '" << output_path << "'.\n";
    return 4;
  }

	return 0;
}
