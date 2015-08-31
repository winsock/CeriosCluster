//
//  RandomSelect.hpp
//  LoginHandler
//
//  Copied by Andrew Querol on 8/30/15
//  https://gist.github.com/cbsmith/5538174
//

#ifndef RandomSelect_hpp
#define RandomSelect_hpp

//Reference implementation for doing random number selection from a container.
//Kept for posterity and because I made a surprising number of subtle mistakes on my first attempt.
#include <random>
#include <iterator>

template <typename RandomGenerator = std::default_random_engine>
struct RandomSelect {
    //On most platforms, you probably want to use std::random_device("/dev/urandom")()
    RandomSelect(RandomGenerator g = RandomGenerator(std::random_device()()))
    : gen(g) {}
    
    template <typename Iter>
    Iter select(Iter start, Iter end) {
        std::uniform_int_distribution<> dis(0, static_cast<std::uniform_int_distribution<>::result_type>(std::distance(start, end)) - 1);
        std::advance(start, dis(gen));
        return start;
    }
    
    //convenience function
    template <typename Iter>
    Iter operator()(Iter start, Iter end) {
        return select(start, end);
    }
    
    //convenience function that works on anything with a sensible begin() and end(), and returns with a ref to the value type
    template <typename Container>
    auto operator()(const Container& c) -> decltype(*begin(c))& {
        return *select(begin(c), end(c));
    }
    
private:
    RandomGenerator gen;
};

#endif /* RandomSelect_hpp */
