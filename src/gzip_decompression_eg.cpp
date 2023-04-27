#include <iostream>
#include <fstream>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/filter/gzip.hpp>

int main()
{
    std::ifstream input_file("nc_out.txt", std::ios::binary);
    std::ofstream output_file("test.csv", std::ios::binary);
    if (!input_file || !output_file)
    {
        std::cerr << "Error: Unable to open file(s)!\n";
        return 1;
    }

    boost::iostreams::filtering_ostream gzip_stream;
    gzip_stream.push(boost::iostreams::gzip_decompressor ());
    gzip_stream.push(output_file);

    gzip_stream << input_file.rdbuf();

    return 0;
}
