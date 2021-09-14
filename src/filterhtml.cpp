// filterhtml.cpp : Defines the entry point for the console application.
//

#include <stdio.h>
#include <string>
#include <iostream>     // std::cout
#include <fstream>      // std::ifstream
#include <vector>
#include <sstream>
#include <algorithm>    // std::min

#include "utils.h"

static std::string WEBSITE_HOSTNAME = "http://www.end2endzone.com";
static bool process_file_in_place = true;

static const char link_reference_endding_characters[] = { '\n', '\0' };
static const size_t num_link_reference_endding_characters = sizeof(link_reference_endding_characters) / sizeof(link_reference_endding_characters[0]);

struct Arguments {
  std::string input_file;
  std::string input_directory;
};

int process_directory(const std::string & input_directory);
int process_file(const std::string & input_file);

void filter_span(std::string & content);
void filter_paragraph_with_custom_css(std::string & content);
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
/// Each paragraph that had custom css classes are wrapped in a shortcode which name matches the name of the class.
/// For example:
///   {{< postedit >}}
///     Source code is now moved to GitHub. Source code can be downloaded from [the project's GitHub page](http://github.com/end2endzone/msbuildreorder).
///   {{< /postedit >}}
/// And the shortcode for postedit is the following:
///   <p class="postedit">
///     {{ .Inner | markdownify }}
///   </p>
/// This filter removes the "<p>" tags but only if there is no html inside the tag.
/// </summary>
void filter_paragraph_with_custom_css(std::string & content) {
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
      // Wrap the content inside a custom shortcode that matches the name of the css class
      
      // Clean up inner text
      trim_html_whitespace(inner_text);

      std::string markdown;
      markdown.append("{{< ");
      markdown.append(class_value);
      markdown.append(" >}}\n");
      markdown.append("  ");
      markdown.append(inner_text + "\n");
      markdown.append("{{< /");
      markdown.append(class_value);
      markdown.append(" >}}\n");
      
      // replace
      content.replace(content.begin() + info.open_start, content.begin() + info.close_end + 1, markdown);
    } else {

      // Clean up inner text
      trim_html_whitespace(inner_text);

      // replace
      content.replace(content.begin() + info.open_start, content.begin() + info.close_end + 1, inner_text);
    }
  }
}

/// <summary>
/// This filter removes all <p> tags.
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
      std::string markdown_emphasized = inner_text;
      
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

    bool has_inner_html = has_inner_html_tags(inner_text);
    if (has_inner_html) {
      // The inner text of the tag has more html inside. Do not proceed with the replacement. Do more passes to replace all the code.
      // Search for the next tag
      offset = info.close_end + 1;
    } else {

      bool is_single_line = (inner_text.find('\n') == std::string::npos);

      // Check if this <pre> tag has a class that would described the content's language.
      // For example: `<pre class="lang:c++ decode:true" title="Arduino tone and delay functions overrides" data-url="http://www.end2endzone.com/wp-content/uploads/2016/10/Arduino-tone-and-delay-functions-overrides.ino">`
      std::string lang;
      std::string class_value = get_html_attribute_value(content, "class", info);
      if (!class_value.empty()) {
        // get the 'lang' parameter
        lang = find_class_property(class_value, "lang");
        if (lang == "default")
          lang = "";
        else if (lang == "c++")
          lang = "cpp";
      }

      // Detect if the code is inlined in the content or if the code is from a local static file
      HTML_ATTRIBUTE_INFO data_url_info;
      bool has_data_url_value = find_html_attribute_boundaries(content, "data-url", info.open_start, info.open_end, data_url_info);
      if (has_data_url_value) {
        is_single_line = false;

        // Read the url of the file and trim the hostname from the url
        std::string data_url_value = get_html_attribute_value(content, "data-url", info);
        search_and_replace(data_url_value, WEBSITE_HOSTNAME, "");

        // Hugo's storage file starts with /static/
        data_url_value.insert(0, "/static");

        // Use a hugo shortcode to highlight file identified by the data-url into this content
        // The file hightlight-static-file.html should have the following content: `{{ highlight (readFile (.Get "file")) (.Get "lang") "" }}`
        // and should be called with the following syntax: `{{< hightlight-static-file file="/static/wp-content/uploads/2015/01/guess.cpp" lang="cpp" >}}`
        std::string markdown_code;
        markdown_code.append("{{< hightlight-static-file file=\"");
        markdown_code.append(data_url_value);
        markdown_code.append("\" lang=\"");
        markdown_code.append(lang);
        markdown_code.append("\" >}}");

        // replace
        content.replace(content.begin() + info.open_start, content.begin() + info.close_end + 1, markdown_code);
      }
      else {
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
}

void filter_known_html_entities(std::string & content) {
  // look for &nbsp; encoded as a utf8 code point
  search_and_replace(content, "\xc2\xa0", "&nbsp;");

  search_and_replace(content, "&#215;", "x");
  search_and_replace(content, "&#8211;", "-");
  search_and_replace(content, "&#8216;", "'");
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
    filter_paragraph_with_custom_css(content);
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

void show_usage() {
  std::cout << "filterhtml\n";
  std::cout << "Usage:\n";
  std::cout << "  Replace html formatting in a markdown file by native markdown syntax.\n";
  std::cout << "Features:\n";
  std::cout << "  The following html tags are supported: span, p, img, a, strong, i, em, code, ul, li, pre, div, table.\n";
  std::cout << "  The function 'filter_paragraph_with_custom_css()' supports paragraph with custom css classes.\n";
  std::cout << "  These paragraphs are converted and a shortcode matching the custom css class is added before and after the paragraph's content.\n";
  std::cout << "  Wordpress galleries are properly identified and converted to a markdown table.\n";
  std::cout << "  Html tables which are missing an header row (or if the header row cannot be identified) have an additional empty row added at the beginning as the header.\n";
  std::cout << "  Most known html entities are replaced by their corresponding utf-8 character.\n";
  std::cout << "  Whitespace is removed as much as possible.\n";
  std::cout << "  Reference-style links are replaced by inline links.\n";
  std::cout << "  Featured_image element in front matter is replaced by image.src format.\n";
  std::cout << "Arguments:\n";
  std::cout << "  --if=<path>\t\tPath to markdown file.\n";
  std::cout << "  --id=<path>\t\tPath to directory with markdown files.\n";
  std::cout << "\n";
}

int process_directory(const std::string & input_directory) {
  if (!dir_exists(input_directory.c_str())) {
    std::cout << "Directory not found: '" << input_directory << "'.\n";
    return 2;
  }

  std::cout << "Reading directory file '" << input_directory << "'.\n";
  std::vector<std::string> files = get_files_in_directory(input_directory.c_str());
  if (files.empty()) {
    std::cout << "Error. No files in directory '" << input_directory << "'.\n";
    return 3;
  }

  std::cout << "Processing " << files.size() << " files in directory.\n";
  for(size_t i=0; i<files.size(); i++) {
    const std::string & file_path = files[i];
    int return_code = process_file(file_path);
    if (return_code != 0) {
      return return_code;
    }
  }

  return 0;
}

int process_file(const std::string & input_file) {
  if (!file_exists(input_file.c_str())) {
    std::cout << "File not found: '" << input_file << "'.\n";
    return 2;
  }

  std::cout << "Loading file '" << input_file << "'.\n";
  std::string content = load_file(input_file.c_str());
  if (content.empty()) {
    std::cout << "Error. Unable to load file '" << input_file << "'.\n";
    return 3;
  }

  run_all_filters(content);

  // use in-place replacement
  std::string output_path;
  if (process_file_in_place)
    output_path = input_file;
  else
    output_path = input_file+".backup.md";

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

int main(int argc, char* argv[])
{
  Arguments args;

  // Show usage if nothing is specified.
  if (argc <= 1) {
    show_usage();
    return 1;
  }

  // Search --if=<file> argument
  // Search --id=<dir> argument
  args.input_file = find_argument("if", argc, argv);
  args.input_directory = find_argument("id", argc, argv);

  if (args.input_file.empty() && args.input_directory.empty()) {
    std::cout << "Error. Please specify --if=<file> or --id=<dir> arguments.\n";
    std::cout << "\n";
    show_usage();
    return 1;
  }

  if (!args.input_file.empty()) {
    int return_code = process_file(args.input_file);
    if (return_code != 0) {
      return return_code;
    }
  }

  if (!args.input_directory.empty()) {
    int return_code = process_directory(args.input_directory);
    if (return_code != 0) {
      return return_code;
    }
  }

	return 0;
}
