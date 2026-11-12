#include <cstddef>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include<array>
#include<span>
#include<vector>
#include<string>
constexpr std::size_t buf_size = 16;

enum class colorwish{
    unset,
    on,
    off
};

namespace color {

    bool enabled = false;
    constexpr const char* OFFSET = "\x1b[38;5;244m";
    constexpr const char* HEX_LO = "\x1b[38;5;153m";
    constexpr const char* HEX_HI = "\x1b[38;5;203m";
    constexpr const char* HEX_NULL = "\x1b[38;5;238m";//dark green
    constexpr const char* ASCII_PR = "\x1b[38;5;120m";
    constexpr const char* ASCII_NP = "\x1b[38;5;238m";//dark greeen
    constexpr const char* HEADER = "\x1b[1;37m";//bold white
    constexpr const char* KEY = "\x1b[38;5;75m";
    constexpr const char* VALUE = "\x1b[38;5;222m";
    constexpr const char* MATCH = "\x1b[1;31m";
    constexpr const char* STRING = "\x1b[38;5;120m";
    constexpr const char* ENTROPY_LO = "\x1b[38;5;46m";  // bright green
    constexpr const char* ENTROPY_MD = "\x1b[38;5;226m"; // yellow     
    constexpr const char* ENTROPY_HI = "\x1b[38;5;196m"; // bright red 
    constexpr const char* RESET     = "\x1b[0m";
}


const char* get(const char* code){
    return color::enabled ? code : "";
}

static void print_hex(std::span<const char> bytes, std::size_t offset) {
    std::cout << std::hex
              << std::setw(8)
              << std::setfill('0')
              << offset
              << "  ";
     for(std::size_t bytes_it = 0; bytes_it < bytes.size(); ++bytes_it) {
        
        if(bytes_it == 8){
            std::cout<<" ";
        }

        const auto byte = static_cast<unsigned char>(bytes[bytes_it]);
        std::cout <<' '
                  << std::setw(2)
                  << static_cast<unsigned int>(byte);
    }
}

static void print_ascii(std::span<const char>bytes){
    
    std::cout<< std::dec << " |";

    for(std::size_t ascii_it = 0; ascii_it < bytes.size(); ++ascii_it){
        const auto byte = static_cast<unsigned char>(bytes[ascii_it]);
        
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

    std::array<char , buf_size> buffer{};
    std::size_t offset = 0;
    std::size_t total_bytes = 0;
    while (true) {
        file.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
        const std::streamsize bytes_read = file.gcount();
        if (bytes_read > 0) {
            const auto count = static_cast<std::size_t>(bytes_read);
            print_hex(std::span<const char>(buffer.data(), count), offset);
            print_ascii(std::span<const char>(buffer.data(), count));
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
    std::cout << std::dec
              << "total bytes read : " << total_bytes << '\n';

    std::cout.flush();

    if(!std::cout){
        std::cerr<<"bytelens : error waiting for output";
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

