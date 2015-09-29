#ifndef CN_FILE_H
#define CN_FILE_H
#include "common.h"
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <boost/iostreams/device/mapped_file.hpp>
using namespace boost::iostreams;

class File {
    mapped_file file;
    uint8_t* data;
    std::unordered_map<hash_t, std::vector<uint8_t*>> chunk_positions;
    std::unordered_set<hash_t> present_chunks;
    void write_chunk(Chunk data, const hash_t& hash, const std::unordered_set<uint8_t*>& skip);
public:
    File(const File&) = delete;
    File& operator=(const File&) = delete;
    File(const std::string& path, size_t resize = 0);
    size_t size() const;
//    uint8_t& operator[](size_t pos);
    std::vector<hash_t> get_chunk_list() const;
    Chunk get_chunk_data(const hash_t& hash) const;
    void write_chunk(Chunk data, const hash_t& hash);
    void set_chunks_from_list(const std::vector<hash_t>& chunks);
    const std::unordered_set<hash_t>& get_present_chunks() const;
    size_t count_total_chunks() const;
    size_t count_present_chunks() const;
};
#endif
