# end2endzone-wp-hugo-migration

A list of tools to help in migrating a WordPress website to Hugo.

# Preface

I migrated my Wordpress blog instance to [Hugo](https://gohugo.io/). For the converting posts to markdown format, I used [SchumacherFM/wordpress-to-hugo-exporter](https://github.com/SchumacherFM/wordpress-to-hugo-exporter).

To my surprise, it seems that my posts, now in markdown format, are still having html formatting. I wish to get all formatting removed and get "plain markdown" without any formatting. At the time, I was preferring to use wordpress visual editor to set my desired formatting. I think that exporting to hugo would be a nice opportunity to start over and set all my formatting with css instead of having all my formatting in my post "raw data".

For example:

- My level 1 headers `<h1>Introduction</h1>` are converted to `# <span id="Introduction">Introduction</span>`. I was expecting to only get `# Introduction`.
- Markdown hyperlinks are exported as html. For example, I get `<a class="download-link" title="Version 1.2.219" href="http://www.end2endzone.com/download/2109/" rel="nofollow">Fade a LED example.ino</a>` instead of the expected `[Fade a LED example.ino](http://www.end2endzone.com/download/2109/)`.
- Multiple lists are exported as html with sequences of `<li style="text-align: justify;">State #1 &#8211; IDLE (1000 ms)</li>` instead of plain markdown with `* State #1 &#8211; IDLE (1000 ms)`.
- Multiple characters in my posts are written as HTML entities instead of the normal utf-8 character even if the character is not written as an HTML entity in my wordpress post.

# Tools

## filterimagesizes

Features:

* Search in a `wp-content` directory for images sizes and delete them leaving only the master image.

* Replaces in posts any reference of a sub image size by the highest resolution image.

* Delete images sub sizes from `wp-content` directory. The highest resolution image is not deleted.

Arguments:

* `--wp-content=<dir>` : Path to a wordpress 'wp-content' directory.

* `--content=<dir>` : Path to the 'content' directory of a Hugo site repository.

## filterhtml

Replace html formatting in a markdown file by native markdown syntax.

Features:

* The following html tags are supported: `span`, `p`, `img`, `a`, `strong`, `i`, `em`, `code`, `ul`, `li`, `pre`, `div`, `table`.
* The function `filter_paragraph_with_custom_css()` supports paragraph with custom css classes which were specific for by blog. These paragraphs are converted and a shortcode matching the custom css class is added before and after the paragraph's content. Replace the list of custom classes by yours to get the same result.
* [Wordpress galleries](https://wordpress.org/support/article/the-wordpress-gallery/) are properly identified and converted to a markdown table.
* Html tables which are missing an header row (or if the header row cannot be identified) have an additional empty row added at the beginning as the header.
* Most known html entities are replaced by their corresponding utf-8 character.
* Whitespace is removed as much as possible.
* [Reference-style links](https://www.markdownguide.org/basic-syntax/#reference-style-links) are replaced by [inline links](https://www.markdownguide.org/basic-syntax/#formatting-links).
* Featured_image element in front matter is replaced by image.src format.

Arguments:

* `--file=<path>` : Path to markdown file.

# Build

The code is in c++. It would have been a better idea to code in python or something more portable than c++ but . The code sould compile file on Windows. Some function may not compile on Linux or macOS but it should not be too difficult to implement on these platforms.

The code can be compiled with [CMake](https://cmake.org/).

On Windows, you can use `ci/windows/build_all_debug.bat` or `build_all_release.bat` for a quick build.

If not, you should be able to generate the project files and build with the following commands:

```batch
mkdir build
cd build
cmake -DCMAKE_GENERATOR_PLATFORM=x64 -T "" ..
cmake --build . --config Release -- -maxcpucount /m
```
