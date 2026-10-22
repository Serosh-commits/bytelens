#include <cstddef>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
constexpr std::size_t buf_size = 16;

static void print_hex(const char* buffer, std::size_t bytes_read,std::size_t offset) {
    std::cout << std::hex
              << std::setw(8)
              << std::setfill('0')
              << offset
              << "  ";
     for(std::size_t bytes_it = 0; bytes_it < bytes_read; ++bytes_it) {
        const auto byte = static_cast<unsigned char>(buffer[bytes_it]);

        std::cout<<" ";
        if (bytes_it != 0) {
            std::cout << ' ';
        }

        if(bytes_it  == 8){
            std::cout<<' ';
        }

        std::cout << std::setw(2)
                  << static_cast<unsigned int>(byte);
    }
}

static void print_ascii(const char* buffer, std::size_t byte_read){
    
    std::cout<< std::dec << " |";

    for(std::size_t ascii_it = 0; ascii_it  < byte_read; ++ascii_it){
        const char byte = static_cast<unsigned char>(buffer[ascii_it]);
        
        if(byte >= 32 && byte <= 126){
            std::cout<<static_cast<char>(byte);
        } else {
            std::cout<<".";
        }
    }

    std::cout<<"|  \n";

}
int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "<include file>\n";
        return EXIT_FAILURE;
    }
    std::ifstream file(argv[1], std::ios::binary);

    if (!file) {
        std::cerr << "bytelens: failed to open file '"
                  << argv[1] << "'\n";
        return EXIT_FAILURE;
    }

    char buffer[buf_size];
    std::size_t offset = 0;
    std::size_t total_bytes = 0;
    while (true) {
        file.read(buffer, sizeof(buffer));
        const std::streamsize bytes_read = file.gcount();
        if (bytes_read > 0) {
            const auto count = static_cast<std::size_t>(bytes_read);
            print_hex(buffer, count, offset);
            print_ascii(buffer, bytes_read);
            offset += count;
            total_bytes += count;
        }

        if (file.bad()) {
            std::cerr << "bytelens: error reading file input\n";
            return EXIT_FAILURE;
        }

        if (file.eof()) {
            break;
        }

        if (file.fail()) {
            std::cerr << "bytelens: error reading the file\n";
            return EXIT_FAILURE;
        }
    }

    if (!std::cout) {
        std::cerr << "bytelens: error writing the  output\n";
        return EXIT_FAILURE;
    }

    std::cout << std::dec
              << "total bytes read : " << total_bytes << '\n';

    return EXIT_SUCCESS;
}

