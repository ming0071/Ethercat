/*****************************************************************************
 *
 *  Copyright (C) 2026 StitchLilo626
 *
 *  This file is part of the IgH EtherCAT Master.
 *
 *  The IgH EtherCAT Master is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License version 2, as
 *  published by the Free Software Foundation.
 *
 *  The IgH EtherCAT Master is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General
 *  Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with the IgH EtherCAT Master; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 ****************************************************************************/

#include "DataTypeHandler.h"

#include <iostream>
#include <string>

class TestDataTypeHandler: public DataTypeHandler
{
    public:
        static std::string convert(const char *name, const std::string &input)
        {
            uint64_t data[8];
            size_t size = interpretAsType(findDataType(name), input,
                    data, sizeof(data));
            return std::string(reinterpret_cast<const char *>(data), size);
        }
};

static bool check(const char *type, const std::string &input,
        const std::string &expected, bool reject = false)
{
    try {
        std::string data = TestDataTypeHandler::convert(type, input);
        if (!reject && data == expected) {
            return true;
        }
    } catch (const std::ios::failure &) {
        if (reject) {
            return true;
        }
    } catch (const std::exception &e) {
        std::cerr << e.what() << std::endl;
    }
    std::cerr << "Unexpected conversion for " << type << ": " << input
        << std::endl;
    return false;
}

int main()
{
    struct {
        const char *type, *input, *expected;
        size_t size;
    } valid[] = {
        {"bool", "1", "\1", 1},
        {"int8", "-128", "\x80", 1},
        {"int16", "-32768", "\0\x80", 2},
        {"int32", "-2147483648", "\0\0\0\x80", 4},
        {"int64", "-9223372036854775808", "\0\0\0\0\0\0\0\x80", 8},
        {"uint8", "255", "\xff", 1},
        {"uint16", "65535", "\xff\xff", 2},
        {"uint32", "4294967295", "\xff\xff\xff\xff", 4},
        {"uint64", "18446744073709551615",
            "\xff\xff\xff\xff\xff\xff\xff\xff", 8},
        {"uint16", "0x1234", "\x34\x12", 2},
        {"uint16", "0123", "\x53\0", 2},
        {"uint16", "+123", "\x7b\0", 2},
        {"int16", " \t-123 \r\n", "\x85\xff", 2},
        {"uint16", " 123 \t\r\n", "\x7b\0", 2},
        {"float", "1.5", "\0\0\xc0\x3f", 4},
        {"double", "1.25e1 \n", "\0\0\0\0\0\0\x29\x40", 8}
    };
    const char *numeric[] = {"bool", "int8", "int16", "int32", "int64",
        "uint8", "uint16", "uint32", "uint64", "float", "double"};
    struct {
        const char *type;
        std::string input;
    } invalid[] = {
        {"uint16", "08"}, {"uint16", "0x12zz"}, {"uint16", "0x"},
        {"float", "1.5oops"}, {"double", "1e+"}, {"bool", "2"},
        {"int8", "128"}, {"int16", "-32769"}, {"int32", "2147483648"},
        {"int64", "9223372036854775808"}, {"uint8", "256"},
        {"uint16", "65536"}, {"uint32", "4294967296"},
        {"uint64", "18446744073709551616"},
        {"uint16", "123abc"}, {"uint16", "123 \txyz"},
        {"uint16", std::string("123\0x", 5)}
    };
    const char *strings[] = {"string", "octet_string", "unicode_string"};
    const std::string text[] = {" 123abc \n", std::string("A\0B", 3), ""};
    unsigned int failures = 0, count = 0;

    for (size_t i = 0; i < sizeof(valid) / sizeof(valid[0]); i++, count++) {
        failures += !check(valid[i].type, valid[i].input,
                std::string(valid[i].expected, valid[i].size));
    }
    for (size_t i = 0; i < sizeof(numeric) / sizeof(numeric[0]); i++) {
        const char *bad[] = {"1junk", "1 2", ""};
        for (size_t j = 0; j < sizeof(bad) / sizeof(bad[0]); j++, count++) {
            failures += !check(numeric[i], bad[j], "", true);
        }
    }
    for (size_t i = 0; i < sizeof(invalid) / sizeof(invalid[0]);
            i++, count++) {
        failures += !check(invalid[i].type, invalid[i].input, "", true);
    }
    for (size_t i = 0; i < sizeof(strings) / sizeof(strings[0]); i++) {
        for (size_t j = 0; j < sizeof(text) / sizeof(text[0]); j++, count++) {
            failures += !check(strings[i], text[j], text[j]);
        }
    }
    std::cout << count << " checks, " << failures << " failures" << std::endl;
    return failures ? 1 : 0;
}
