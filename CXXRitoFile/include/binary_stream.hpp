#pragma once
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cstring>
#include <cstdint>
#include <sstream>

#include "structs.hpp"

namespace RitoFile {
    class BinaryReader {
    public:
        unsigned int size_offsets = 0;
        std::istream& stream;

        BinaryReader(std::istream& s) : stream(s) {}

        inline size_t tell() { return stream.tellg(); }
        inline void seek(size_t pos, std::ios_base::seekdir dir = std::ios::beg) { stream.seekg(pos, dir); }
        inline void pad(size_t len) { stream.seekg(len, std::ios::cur); }

        inline std::vector<char> read(size_t length) {
            std::vector<char> buffer(length);
            stream.read(buffer.data(), length);
            return buffer;
        }


        template <typename T>
        inline void readBuffered(std::vector<T>& buffer, size_t byte_count) {
            if (byte_count % sizeof(T) != 0) {
                throw std::invalid_argument("Byte count must be a multiple of the size of T");
            }
            size_t element_count = byte_count / sizeof(T);
            buffer.resize(element_count);
            stream.read(reinterpret_cast<char*>(buffer.data()), byte_count);
            if (!stream.good()) {
                throw std::runtime_error("Error reading from stream");
            }
        }

        template<typename T>
        inline T readScalar() {
            T val;
            stream.read(reinterpret_cast<char*>(&val), sizeof(T));
            return val;
        }

        inline bool readBool() { return readScalar<bool>(); }
        inline std::int8_t readI8() { return readScalar<int8_t>(); }
        inline std::uint8_t readU8() { return readScalar<uint8_t>(); }
        inline std::int16_t readI16() { return readScalar<int16_t>(); }
        inline std::uint16_t readU16() { return readScalar<uint16_t>(); }
        inline std::int32_t readI32() { return readScalar<int32_t>(); }
        inline std::uint32_t readU32() { return readScalar<uint32_t>(); }
        inline std::int64_t readI64() { return readScalar<int64_t>(); }
        inline std::uint64_t readU64() { return readScalar<uint64_t>(); }
        inline float readF32() { return readScalar<float>(); }
        inline double readF64() { return readScalar<double>(); }
        inline Matrix4 readMtx4() { return readScalar<Matrix4>(); }

        inline std::string readString(size_t length) {
            std::string str(length, '\0');
            stream.read(&str[0], length);
            return str;
        }

        inline std::string readStringNullTerminated() {
            std::string str;
            char ch;
            while (stream.get(ch) && ch != '\0') {
                str += ch;
            }
            return str;
        }

        inline std::string readStringPadded(int lenght) {
            std::string str;
            str.reserve(lenght);

            char ch;
            for (int i = 0; i < lenght; i++) {
                stream.get(ch);
                if (ch != '\0') {
                    str += ch;
                }
            }
            return str;
        }

        inline std::string readStringSized16() {
            uint16_t size = readU16();
            return readString(size);
        }

        inline std::string readStringSized32() {
            uint32_t size = readU32();
            return readString(size);
        }

        template<template<typename> class Container, typename Base, size_t N, size_t... Is>
        inline Container<Base> readContainerImpl(std::index_sequence<Is...>) {
            Base values[N];
            stream.read(reinterpret_cast<char*>(values), sizeof(Base) * N);
            return Container<Base>(values[Is]...);
        }

        template<template<typename> class Container, typename Base, size_t N>
        Container<Base> readContainer() {
            return readContainerImpl<Container, Base, N>(std::make_index_sequence<N>{});
        }
    };

    class BinaryWriter {
        std::ostringstream& stream;

    public:
        BinaryWriter(std::ostringstream& s) : stream(s) {}

        size_t tell() { return stream.tellp(); }
        void seek(size_t pos, std::ios_base::seekdir dir = std::ios::beg) { stream.seekp(pos, dir); }

        void write(const std::vector<char>& data) {
            stream.write(data.data(), data.size());
        }

        template<typename T>
        void writeScalar(T value) {
            stream.write(reinterpret_cast<const char*>(&value), sizeof(T));
        }

        void writeBool(bool val) { writeScalar<bool>(val); }
        void writeI8(int8_t val) { writeScalar<int8_t>(val); }
        void writeU8(uint8_t val) { writeScalar<uint8_t>(val); }
        void writeI16(int16_t val) { writeScalar<int16_t>(val); }
        void writeU16(uint16_t val) { writeScalar<uint16_t>(val); }
        void writeI32(int32_t val) { writeScalar<int32_t>(val); }
        void writeU32(uint32_t val) { writeScalar<uint32_t>(val); }
        void writeI64(int64_t val) { writeScalar<int64_t>(val); }
        void writeU64(uint64_t val) { writeScalar<uint64_t>(val); }
        void writeF32(float val) { writeScalar<float>(val); }
        void writeF64(double val) { writeScalar<double>(val); }

        void writeString(const std::string& str) {
            stream.write(str.c_str(), str.size());
        }

        void writeStringPadded(const std::string& str, size_t length) {
            size_t len = std::min(str.size(), length);
            stream.write(str.c_str(), len);
            for (size_t i = len; i < length; ++i)
                stream.put('\0');
        }

        void writeStringSized16(const std::string& str) {
            uint16_t size = static_cast<uint16_t>(str.size());
            writeU16(size);
            writeString(str);
        }

        void writeStringSized32(const std::string& str) {
            uint32_t size = static_cast<uint32_t>(str.size());
            writeU32(size);
            writeString(str);
        }

        void writeCStringSep0(const std::string& str) {
            for (char c : str) {
                stream.put(c);
                stream.put('\0');
            }
        }

        template<template<typename> class Container, typename Base, size_t N>
        void writeContainer(const Container<Base>& container) {
            stream.write(reinterpret_cast<const char*>(&container), sizeof(Base) * N);
        }
    };
}