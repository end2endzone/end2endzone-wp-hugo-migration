// markdownfilter.cpp : Defines the entry point for the console application.
//

#include <stdio.h>
#include <string>
#include <iostream>     // std::cout
#include <fstream>      // std::ifstream
#include <vector>
#include <sstream>
#include <algorithm>    // std::min, std::sort

#include "utils.h"

static std::string WEBSITE_HOSTNAME = "http://www.end2endzone.com";

bool my_string_sorting_function (std::string i, std::string j) { return (i<j); }

struct Arguments {
  std::string wp_content_dir;
  std::string content_dir;
};

struct Context {
  std::vector<std::string> image_files;
  std::vector<std::string> posts_files;
};

struct ImageCount {
  size_t master;
  std::vector<size_t> sizes;
};

void filter_img_srcset(std::string & content) {
  static const std::string search_pattern1 = "SRCSET=\"";
  
  size_t pos_start = content.find(search_pattern1);
  while(pos_start != std::string::npos) {
    size_t next_pos = pos_start; // define the position where the next search offset

    // Search the end of the srcset string
    size_t pos_end = content.find("\"", pos_start+search_pattern1.size());
    if (pos_end != std::string::npos) {
      content.erase(content.begin()+pos_start, content.begin()+pos_end);
      // next search starts at same position
    }
    else {
      next_pos = pos_start + search_pattern1.size();
    }

    // Search again
    pos_start = content.find(search_pattern1, next_pos);
  }
}

ImageCount get_image_usage_count(const std::string & master_path, const std::vector<std::string> & image_sizes, const Context & c) {
  ImageCount usage;

  // Zeroize output
  usage.master = 0;
  usage.sizes.clear();
  for(size_t i=0; i<image_sizes.size(); i++) {
    usage.sizes.push_back(0);
  }

  // Search all posts for all image files
  for(size_t i=0; i<c.posts_files.size(); i++) {
    const std::string & post_path = c.posts_files[i];

    std::string post_content = load_file(post_path);
    uppercase(post_content);

    // Remove multiple sources from <img> tags
    filter_img_srcset(post_content);

    // Search for master image
    std::string master_file_name_ext = get_file_name_with_extension(master_path.c_str());
    uppercase(master_file_name_ext);
    if (post_content.find(master_file_name_ext) != std::string::npos)
      usage.master++;

    // Check all image sizes
    for(size_t j=0; j<image_sizes.size(); j++) {
      const std::string & image_size_path = image_sizes[j];
      std::string image_size_file_name_ext = get_file_name_with_extension(image_size_path.c_str());
      uppercase(image_size_file_name_ext);
      if (post_content.find(image_size_file_name_ext) != std::string::npos)
        usage.sizes[j]++;
    }
  }

  return usage;
}

int sanitize_posts(const std::string & master_path, const std::vector<std::string> & image_sizes, const Context & c) {
  // Search all posts for all image files
  for(size_t i=0; i<c.posts_files.size(); i++) {
    const std::string & post_path = c.posts_files[i];

    std::string post_content = load_file(post_path);
    std::string post_content_backup = post_content;

    // Search for master image
    std::string master_file_name_ext = get_file_name_with_extension(master_path.c_str());

    // Replaces all sub images by the original filename
    bool corrected = false; // bit to know if the content of the post was modified
    for(size_t j=0; j<image_sizes.size(); j++) {
      const std::string & image_size_path = image_sizes[j];
      std::string image_size_file_name_ext = get_file_name_with_extension(image_size_path.c_str());

      search_and_replace(post_content, image_size_file_name_ext, master_file_name_ext);;
      corrected = true;
    }

    if (corrected) {
      // Check if the post has really changed
      if (post_content != post_content_backup) {
        // Save the modified content to file
        bool saved = save_file(post_path.c_str(), post_content);
        if (!saved) {
          std::cout << "Error. Failed to save file: " << post_path << "\n";
          return 4;
        }
      }
    }
  }

  return 0;
}

int search_image_sizes(const Arguments & args, const Context & c) {
  std::cout << "Searching for master image files...\n";

  for(size_t i=0; i<c.image_files.size(); i++) {
    const std::string & master_path = c.image_files[i];

    // Build the file prefix search pattern
    std::string master_image_ext = get_file_extension(master_path.c_str());
    std::string file_prefix;
    if (master_image_ext.empty()) {
      file_prefix = master_path + "-";
    }
    else {
      file_prefix = master_path.substr(0, master_path.size() - 1 - master_image_ext.size());
    }

    std::string master_parent_path = get_parent_directory(master_path.c_str());

    // Search for image sizes(files that start with the same filename pattern) in the same directory.
    std::vector<std::string> sizes;
    std::vector<std::string> candidate_files = get_files_in_directory(master_parent_path.c_str());

    for(size_t j=0; j<candidate_files.size(); j++) {
      const std::string & test_path = candidate_files[j];

      // Is this really an image sub size of the original file ?
      bool is_image_size = is_sub_image_size(master_path.c_str(), test_path.c_str());
      if (is_image_size) {
        std::cout << "Found sub image size: " << test_path << "\n";
        sizes.push_back(test_path);
      }
    }

    if (sizes.empty())
      continue;

    ImageCount count = get_image_usage_count(master_path, sizes, c);

    // Display counts
    std::string master_file_name_ext = get_file_name_with_extension(master_path.c_str());
    std::cout << "Count for " << master_file_name_ext << ": " << count.master << "\n";
    for(size_t j=0; j<sizes.size(); j++) {
      const std::string & image_size_path = sizes[j];
      std::string image_size_file_name_ext = get_file_name_with_extension(image_size_path.c_str());
      std::cout << "Count for " << image_size_file_name_ext << ": " << count.sizes[j] << "\n";
    }

    std::cout << "Sanitizing posts...\n";
    int returncode = sanitize_posts(master_path, sizes, c);
    if (returncode != 0)
      return returncode;
    std::cout << "sanitized\n";

    // Delete sub file image sizes
    for(size_t j=0; j<sizes.size(); j++) {
      const std::string & image_size_path = sizes[j];
      bool deleted = delete_file(image_size_path.c_str());
      if (!deleted)
        return 5;
    }

    // next file in list
  }

  return 0;
}

void show_usage() {
  std::cout << "FilterImageSizes\n";
  std::cout << "Usage:\n";
  std::cout << "  Search in a wp-content directory for images sizes and delete them leaving only the master image.\n";
  std::cout << "  Replaces in all posts any reference of a sub image size by the image with the highest resolution.\n";
  std::cout << "  Delete images sub sizes from wp-content directory. The highest resolution image is not deleted.\n";
  std::cout << "Arguments:\n";
  std::cout << "  --wp-content=<dir>\t\tPath to a wordpress 'wp-content' directory.\n";
  std::cout << "  --content=<dir>\t\tPath to the 'content' directory of a hugo site repository.\n";
  std::cout << "\n";
}

int main(int argc, char* argv[])
{
  Arguments args;

  // Show usage if nothing is specified.
  if (argc <= 1) {
    show_usage();
    return 1;
  }

  // Search --wp-content=<dir> argument
  args.wp_content_dir = find_argument("wp-content", argc, argv);
  if (args.wp_content_dir.empty()) {
    std::cout << "Error. Please specify --wp-content=<dir> argument.\n";
    std::cout << "\n";
    show_usage();
    return 1;
  }

  // Search --content=<dir> argument
  args.content_dir = find_argument("content", argc, argv);
  if (args.content_dir.empty()) {
    std::cout << "Error. Please specify --content=<dir> argument.\n";
    std::cout << "\n";
    show_usage();
    return 1;
  }

  // Check that input directories exists
  if (!dir_exists(args.wp_content_dir.c_str())) {
    std::cout << "Error. Directory not found: " << args.wp_content_dir << "\n";
    return 2;
  }
  if (!dir_exists(args.content_dir.c_str())) {
    std::cout << "Error. Directory not found: " << args.content_dir << "\n";
    return 2;
  }

  // Read files from directories
  Context c;

  // Read images
  std::cout << "Reading files from directory: " << args.wp_content_dir << "\n";
  c.image_files = get_files_in_directory(args.wp_content_dir.c_str());
  if (c.image_files.empty()) {
    std::cout << "Error. Directory is empty: " << args.wp_content_dir << "\n";
    return 3;
  }
  std::cout << "Found " << c.image_files.size() << " files in directory.\n";

  // Read posts
  std::cout << "Reading files from directory: " << args.content_dir << "\n";
  c.posts_files = get_files_in_directory(args.content_dir.c_str());
  if (c.posts_files.empty()) {
    std::cout << "Error. Directory is empty: " << args.content_dir << "\n";
    return 3;
  }
  std::cout << "Found " << c.posts_files.size() << " files in directory.\n";

  // Sorting files to make sure we browse files in the expected order
  std::sort (c.image_files.begin(), c.image_files.end(), my_string_sorting_function);
  std::sort (c.posts_files.begin(), c.posts_files.end(), my_string_sorting_function);

  // Start the search
  int result = search_image_sizes(args, c);
  return result;
}
