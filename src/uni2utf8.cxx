/*\
 * utf8-2-unicode.cxx
 *
 * Copyright (c) 2015 - Geoff R. McLane
 * Licence: GNU GPL version 2
 *
\*/
/*\
 * from : http://stackoverflow.com/questions/18534494/convert-from-utf-8-to-unicode-c
 * Per FR #4 seems this app should do more, like read from a utf16 file, and convert...
 * some example code : https://stackoverflow.com/questions/20419605/how-to-convert-unicode-code-points-to-utf-8-in-c
\*/
#include <iostream>
#include <deque>
#include <stdio.h>
#include <stdint.h>


#include <iostream>
#include <locale>
#include <vector>

static const char *module = "utf16-2-utf8";

int test1_main()
{
    int iret = 0;
    typedef std::codecvt<wchar_t, char, mbstate_t> Convert;
    std::wstring w = L"\u20ac\u20ab\u20ac";
    //std::locale locale("en_GB.utf8");
    std::locale locale("en");
    const Convert& convert = std::use_facet<Convert>(locale);

    std::mbstate_t state;
    const wchar_t* from_ptr;
    char* to_ptr;
    std::vector<char> result(3 * w.size() + 1, 0);
    Convert::result convert_result = convert.out(state,
        w.c_str(), w.c_str() + w.size(), from_ptr,
        result.data(), result.data() + result.size(), to_ptr);

    if (convert_result == Convert::ok) {
        std::cout << result.data() << std::endl;
    }
    else {
        iret = 1;
        std::cout << "Failure: " << convert_result << std::endl;
    }
    return iret;
}


#define MY_MX_BUF   16

std::deque<int> unicode_to_utf8(int charcode)
{
    std::deque<int> d;
    if (charcode < 128)
    {
        d.push_back(charcode);
    }
    else
    {
        int first_bits = 6; 
        const int other_bits = 6;
        int first_val = 0xC0;
        int t = 0;
        while (charcode >= (1 << first_bits))
        {
            {
                t = 128 | (charcode & ((1 << other_bits)-1));
                charcode >>= other_bits;
                first_val |= 1 << (first_bits);
                first_bits--;
            }
            d.push_front(t);
        }
        t = first_val | charcode;
        d.push_front(t);
    }
    return d;
}


int utf8_to_unicode(std::deque<int> &coded)
{
    int charcode = 0;
    int t = coded.front();
    coded.pop_front();
    if (t < 128)
    {
        return t;
    }
    int high_bit_mask = (1 << 6) -1;
    int high_bit_shift = 0;
    int total_bits = 0;
    const int other_bits = 6;
    while((t & 0xC0) == 0xC0)
    {
        t <<= 1;
        t &= 0xff;
        total_bits += 6;
        high_bit_mask >>= 1; 
        high_bit_shift++;
        charcode <<= other_bits;
        charcode |= coded.front() & ((1 << other_bits)-1);
        coded.pop_front();
    } 
    charcode |= ((t >> high_bit_shift) & high_bit_mask) << total_bits;
    return charcode;
}

int main()
{
    int iret = 0;
    int charcode, off; 
    uint8_t chars[MY_MX_BUF+2];
    //iret = test1_main();
    for(;;)
    {
        std::cout << "Enter unicode decimal value : 0, or alpha, to exit" << std::endl;
        if (!(std::cin >> charcode)) {
            std::cout << "Failed to get an integer!" << std::endl;
            break;
        }
        if (charcode == 0)
            break;
        std::deque<int> x = unicode_to_utf8(charcode);
        //for(auto c : x)
        off = 0;
        for (std::deque<int>::iterator it = x.begin(); it != x.end(); it++)
        {
            int c = *it;
            std::cout << "\\x" << std::hex << c << " ";
            if (off < MY_MX_BUF) 
                chars[off++] = (uint8_t)c;
        }
        chars[off] = 0;
        std::cout << "disp: '" << chars << "'";
        std::cout << std::endl;
        int c = utf8_to_unicode(x);
        std::cout << "reversed : " << std::dec << c << std::hex << " in hex:" << c << std::endl;
    }
    return iret;
}

// eof = utf16-2-utf8.cxx

