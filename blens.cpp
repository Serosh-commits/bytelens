#include <cstddef>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include<array>
#include<cstdint>
#include<span>
#include<vector>
#include<fcntl.h>
#include<sys/wait.h>
#include<sys/stat.h>
#include<unistd.h>
#include<cstdio>
#include<string>
#include<utility>
#include<algorithm>
#include<string_view>
#include<cstring>
constexpr std::size_t buf_size = 16;
constexpr std::size_t IO_BUFF = 64 * 1024;

enum class colorwish{
    unset,
    on,
    off
};

struct options{
    std::string command;
    std::string filepath;
    uint64_t offset = 0;
    uint64_t limit = UINT64_MAX;
    int width = 16;
    int min_strlen = 4;
    std::vector<uint8_t>raw;
    std::string pattern_raw;
    //kept it unset the color for now so user can choose what to select
    colorwish color_wish = colorwish::unset;
};

struct file_reader{
    int fd = -1;
    uint64_t pos = 0;
    uint64_t size = 0;
    bool at_eof = false;
    std::array<uint8_t, IO_BUFF>buf;
    std::size_t buf_len = 0;
    std::size_t buf_pos = 0;
    
    file_reader() = default;
    ~file_reader(){
        if(fd >= 0){
            ::close(fd);
        }
    }
    
    file_reader(const file_reader&) = delete;
    file_reader& operator = (const file_reader&) = delete;
    
    file_reader(file_reader&& other) noexcept
        :fd(other.fd),
         pos(other.pos),
         size(other.size),
         at_eof(other.at_eof),
         buf(std::move(other.buf)),
         buf_len(other.buf_len),
         buf_pos(other.buf_pos)
    {
        other.fd = -1;
        other.buf_len = 0;
        other.buf_pos = 0;
    }
    //reading bytes into the buffer 
    std::size_t read(uint8_t* out , std::size_t max){
      std::size_t total = 0;

      while(total < max && !at_eof){

        if (buf_pos >= buf_len) {
          ssize_t n = ::read(fd, buf.data(), IO_BUFF);

          if (n <= 0) {
            at_eof = true;
            break;
          }

          buf_len = static_cast<std::size_t>(n);
          buf_pos = 0;
        }

        std::size_t available = buf_len - buf_pos;
        //give the user only the number he wants so use std::min
        std::size_t take = std::min(available, max - total);
        std::memcpy(out + total, buf.data() + buf_pos, take);
        
        buf_pos = buf_pos + take;
        pos = pos + take;
        total = total + take;
      }

      return total;
    }

    template<typename T>
    bool read_at(uint64_t off, T& out) const {
      ssize_t n = ::pread(fd,&out,sizeof(T),static_cast<off_t>(off));

      return n == static_cast<ssize_t>(sizeof(T));
    }
    bool seek(uint64_t off){
      if (::lseek(fd, static_cast<off_t>(off), SEEK_SET) == (off_t) - 1) {
        return false;
      }

      pos = off;
      buf_len = 0;
      buf_pos = 0;
      at_eof = false;

      return true;
    }
    static file_reader open(const std::string& path){
        file_reader fr;
        fr.fd = ::open(path.c_str(), O_RDONLY);

        if(fr.fd  < 0 ){
            std::perror("bytelens: open");
            std::exit(EXIT_FAILURE);
        }

        struct stat st{};

        if(::fstat(fr.fd, &st) == 0){
            fr.size = static_cast<uint64_t>(st.st_size);
        }

        return fr;
    }
};

namespace color {

    bool enabled = true;
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
    
    const char* get(const char* code){
      return enabled ? code : "";
    }
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
        const char* col = (byte == 0x00) ? color::HEX_NULL:
                          (byte >= 0x20 && byte < 0x7f) ? color::HEX_LO: color::HEX_HI;
                           
        std::cout <<color::get(col)
                  <<' '
                  << std::setw(2)
                  << static_cast<unsigned int>(byte)
                  <<color::get(color::RESET);
    }

    for(std::size_t padding_it = bytes.size(); padding_it < buf_size; ++padding_it){
        
        if(padding_it == buf_size / 2){
            std::cout<< ' ';
        }

        std::cout<<"   ";
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

static bool is_command(std::string_view value){
  static constexpr std::string_view commands[] = {
    "hex",
    "strings",
    "info",
    "headers",
    "entropy",
    "search"
  };

  for (std::string_view cmd : commands) {
    
    if (value == cmd) {
      return true;
    }
  }

  return false;
}
int main(int argc, char* argv[]){
    if (argc < 2) {
        std::cerr << "Usage : bytelens [command] <include file>\n";
        return EXIT_FAILURE;
    }
    
    options opts;
    if (is_command(argv[1])) {
      if (argc < 3) {
        std::cerr<<"ERROR : missing file path :)\n";
        return EXIT_FAILURE;
      }

      opts.command = argv[1];
      opts.filepath = argv[2];
    } else {
      opts.command = "hex";
      opts.filepath = argv[1];
    }
    
    file_reader fr = file_reader::open(opts.filepath);

    if (opts.offset > 0) {
      if (opts.offset >= fr.size) {
        std::cerr<<"offset is outside of the file\n";
        return EXIT_FAILURE;
      }

      fr.seek(opts.offset);
    }
    
    std::array<char , buf_size> buffer{};
    std::size_t offset = 0;
    std::size_t total_bytes = 0;
    while (true) {
      std::size_t bytes_read = fr.read(reinterpret_cast<uint8_t*>(buffer.data()), buffer.size());

        if (bytes_read == 0) {
          break;
        }

      const auto count = static_cast<std::size_t>(bytes_read);
      print_hex(std::span<const char>(buffer.data(), count), offset);
      print_ascii(std::span<const char>(buffer.data(), count));

      offset += count;
      total_bytes += count;
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

