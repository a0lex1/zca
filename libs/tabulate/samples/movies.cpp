#include <tabulate/table.hpp>
#include <tabulate/markdown_exporter.hpp>

using namespace tabulate;
using Row_t = Table::Row_t;

int main() {
  Table movies;
  movies.add_row(Row_t{"S/N", "Movie Name", "Director", "Estimated Budget", "Release Date"});
  movies.add_row(Row_t{"tt1979376", "Toy Story 4", "Josh Cooley", "$200,000,000", "21 June 2019"});
  movies.add_row(Row_t{"tt3263904", "Sully", "Clint Eastwood", "$60,000,000", "9 September 2016"});
  movies.add_row(
      {"tt1535109", "Captain Phillips", "Paul Greengrass", "$55,000,000", " 11 October 2013"});
  
  // center align 'Director' column
  movies.column(2).format().font_align(FontAlign::center);

  // right align 'Estimated Budget' column
  movies.column(3).format().font_align(FontAlign::right);

  // right align 'Release Date' column
  movies.column(4).format().font_align(FontAlign::right);

  MarkdownExporter exporter;
  std::string markdown = exporter.dump(movies);

  std::cout << markdown << std::endl;
}