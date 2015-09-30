#include "file.h"
#include "hash.h"
#include <boost/filesystem.hpp>
#include <stdio.h>
using namespace boost::filesystem;

File::File(const std::string& path, size_t resize) {
    mapped_file_params params;
    params.path = path;
    params.flags = mapped_file::mapmode::readwrite;
    if (resize != 0) {
        if (!exists(path)) {
            params.new_file_size = resize;
        } else {
            resize_file(path, resize);
        }
    }
    file.open(params);
    data = (uint8_t*) file.data();
}

size_t File::size() const {
    return file.size();
}

void File::write_chunk(Chunk data, const hash_t& hash, const std::unordered_set<uint8_t*>& skip) {
    for (auto x: chunk_positions[hash]) {
        if (skip.count(x)) continue;
        memcpy(x, data.data, data.size);
    }
    present_chunks.insert(hash);
}

std::vector<hash_t> File::get_chunk_list() const {
    std::vector<hash_t> hashes;
    for (uint8_t* ptr=data; ptr<data+size(); ptr += chunk_max_size) {
        Chunk chunk(ptr, std::min(ptr+chunk_max_size, data+size()));
        hashes.push_back(chunk.get_hash());
    }
    return hashes;
}

Chunk File::get_chunk_data(const hash_t& hash) const {
    for (auto x: chunk_positions.at(hash)) {
        return {x, std::min(x+chunk_max_size, data+size())};
    }
    return {(size_t)0, nullptr};
}

void File::write_chunk(Chunk data, const hash_t& hash) {
    write_chunk(data, hash, {});
}

void File::set_chunks_from_list(const std::vector<hash_t>& chunks) {
    for (size_t i=0; i<chunks.size(); i++) {
        chunk_positions[chunks[i]].push_back(data+chunk_max_size*i);
    }
    const auto& old_chunks = get_chunk_list();
    if (chunks == old_chunks) {
        for (auto& hash: chunks) {    
            present_chunks.insert(hash);
        }
        return;
    }
    std::unordered_map<hash_t, std::unordered_set<uint8_t*>> already_present;
    for (size_t i=0; i<old_chunks.size(); i++) {
        already_present[old_chunks[i]].insert(data+chunk_max_size*i);
    }
    for (size_t i=0; i<chunks.size(); i++) {
        chunk_positions[chunks[i]].push_back(data+chunk_max_size*i);
    }
    std::unordered_set<uint32_t> weak_present;
    for (auto& x: chunks) {
        weak_present.insert(x.weak_hash);
    }
    for (size_t pos=0; pos<size(); pos++) {
        Hasher hasher(chunk_max_size);
        hasher.update(data+pos, std::min(data+pos+chunk_max_size, data+size()));
        pos += chunk_max_size;
        while (pos < size() && (!weak_present.count(hasher.get_weak_hash()) || !chunk_positions.count(hasher.get_strong_hash()))) {
            hasher.update(data+pos, data+pos+1);
            pos++;
        }
        if (chunk_positions.count(hasher.get_strong_hash())) {
            write_chunk(hasher.get_chunk(), hasher.get_strong_hash(), already_present[hasher.get_strong_hash()]);
            pos -= chunk_max_size;
        }
    }
}

const std::unordered_set<hash_t>& File::get_present_chunks() const {
    return present_chunks;
}

size_t File::count_total_chunks() const {
    return chunk_positions.size();
}

size_t File::count_present_chunks() const {
    return present_chunks.size();
}
