//
//  hexstring.hpp
//  Client Gateway
//
//  Shamelessly found on StackOverflow
//  http://stackoverflow.com/a/30324373
//

#ifndef hexstring_hpp
#define hexstring_hpp

#include <string>
#include <sstream>
#include <iomanip>

template<typename TInputIter>
std::string make_hex_string(TInputIter first, TInputIter last, bool use_uppercase = true, bool insert_spaces = false)
{
    std::ostringstream ss;
    ss << std::hex << std::setfill('0');
    if (use_uppercase)
        ss << std::uppercase;
    while (first != last)
    {
        ss << std::setw(2) << static_cast<int>(*first++);
        if (insert_spaces && first != last)
            ss << " ";
    }
    return ss.str();
}

#endif /* hexstring_hpp */
