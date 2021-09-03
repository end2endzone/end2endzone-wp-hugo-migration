#include "utils.h"

static bool is_utf8_source = true; //change this to false if you are parsing from Windows code page 1252 (Cp1252) or from ISO8859-1 character encoding

static const char html_tag_endding_characters[] = { ' ', '/', '\"', '\'', '>', '\0' };
static const size_t num_html_tag_endding_characters = sizeof(html_tag_endding_characters) / sizeof(html_tag_endding_characters[0]);

bool is_digit(const char c) {
  return (c >= '0' && c <= '9');
}

bool is_letter(const char c) {
  return (  (c >= 'a' && c <= 'z') ||
            (c >= 'A' && c <= 'Z')  );
}

bool is_alphanumeric(const char c) {
  return (is_digit(c) || is_letter(c));
}

bool is_numeric(const char * value) {
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

size_t count_character(const char * text, char character) {
  size_t count = 0;
  while(text[0] != '\0') {
    if (text[0] == character)
      count++;
    text++;
  }
  return count;
}

size_t get_line_indentation(const char * content, size_t offset) {
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

size_t string_length_utf8(const char * text) {
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
size_t string_length_utf8(const std::string & text) {
  return string_length_utf8(text.c_str());
}

size_t string_length(const std::string & text) {
  size_t length = 0;
  if (is_utf8_source)
    length = string_length_utf8(text);
  else
    length = text.size();
  return length;
}

std::string string_center(const std::string & text, size_t length) {
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

std::string string_lengthen(const std::string & text, size_t length) {
  std::string output = text;
  output.reserve(length);
  size_t text_length = string_length(text);
  if (text_length < length) {
    size_t diff = length - text_length;
    output.append(diff, ' ');
  }
  return output;
}

size_t get_table_column_count(const HtmlTable & table) {
  size_t columns = 0;
  for(size_t i=0; i<table.rows.size(); i++) {
    const HtmlTableRow & row = table.rows[i];
    if (columns < row.size())
      columns = row.size();
  }
  return columns;
}

size_t get_table_column_width(const HtmlTable & table, size_t column_index) {
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

std::string to_markdown(const HtmlTable & table) {
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

EOL_TYPE normalize_newlines(std::string & content) {
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

void restore_newlines(std::string & content, EOL_TYPE eol_type) {
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

void search_and_replace(std::string & content, const std::string & token, const std::string & value) {
  if (token.empty())
    return;

  size_t pos = content.find(token, 0);
  while (pos != std::string::npos) {
    content.replace(content.begin() + pos, content.begin() + pos + token.size(), value);

    // next tag
    pos = content.find(token, pos + value.size());
  }
}

std::string load_file(const std::string & path) {
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

bool save_file(const std::string & path, const std::string & content) {
  std::ofstream file(path, std::ios::binary | std::ios::ate);
  if (file.write(content.c_str(), content.size())) {
    return true;
  }
  return false;
}

bool file_exists(const std::string & name) {
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

std::string get_html_attribute_value(const std::string & content, const std::string & attr_name, const HTML_TAG_INFO & tag_info) {
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

size_t get_html_tag_indentation(const char * content, const HTML_TAG_INFO & tag_info) {
  size_t indent = get_line_indentation(content, tag_info.open_start);
  return indent;
}

bool is_any(char c, const std::vector<char> & characters) {
  for (size_t i = 0; i < characters.size(); i++) {
    if (characters[i] == c)
      return true;
  }
  return false;
}

bool is_any(char c, const char * characters, size_t num_characters) {
  for (size_t i = 0; i < num_characters; i++) {
    if (characters[i] == c)
      return true;
  }
  return false;
}

std::string read_until(const std::string & content, size_t offset, const std::vector<char> & stop_characters) {
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

std::string read_until(const std::string & content, size_t offset, const char * characters, size_t num_characters) {
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

std::string read_html_tag(const std::string & content, size_t offset) {
  std::string output;
  output = read_until(content, offset, html_tag_endding_characters, num_html_tag_endding_characters);
  return output;
}

bool has_inner_html_tags(const std::string & content, int ignore_html_tag_flags) {
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

bool has_inner_html_tags(const HtmlTableRow & row, int ignore_html_tag_flags) {
  for(size_t i=0; i<row.size(); i++) {
    const HtmlTableCell & cell = row[i];
    if (has_inner_html_tags(cell, ignore_html_tag_flags))
      return true;
  }
  return false;
}

bool has_inner_html_tags(const HtmlTable & table, int ignore_html_tag_flags) {
  for(size_t i=0; i<table.rows.size(); i++) {
    const HtmlTableRow & row = table.rows[i];
    if (has_inner_html_tags(row, ignore_html_tag_flags))
      return true;
  }
  return false;
}

bool has_cell_spanning(const std::string & content) {
  if (content.find("rowspan") != std::string::npos) {
    return true;
  } else if (content.find("colspan") != std::string::npos) {
    return true;
  }
  return false;
}

std::vector<std::string> split(const std::string & content, char split_character) {
  std::stringstream input(content);
  std::string tmp;
  std::vector<std::string> elements;

  while(std::getline(input, tmp, split_character))
  {
     elements.push_back(tmp);
  }

  return elements;
}

std::string join(const std::vector<std::string> & lines, char join_character) {
  std::string output;
  for(size_t i=0; i<lines.size(); i++) {
    const std::string & line = lines[i];
    if (i > 0)
      output.append(1, join_character);
    output.append(line);
  }
  return output;
}

size_t trim(std::string & text, char trim_character) {
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

size_t trim(std::string & text, const char * trim_str) {
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

size_t trim_html_whitespace(std::string & text) {
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

size_t trim_html_whitespace(HtmlTable & table) {
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

bool is_html_white_character(const std::string & content, size_t offset) {
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

size_t get_first_nonwhite_html_character(const char * content, size_t offset) {
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

void decrease_indent(std::vector<std::string> & lines, size_t num_spaces) {
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

std::string get_line_at_offset(std::string & content, size_t offset) {
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
