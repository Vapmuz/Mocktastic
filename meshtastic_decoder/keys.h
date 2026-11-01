
#ifndef KEYS_H
#define KEYS_H


#include <string>

/**
 * Configuration constants for Meshtastic channel and user keys.
 *
 * Keys are stored as hex strings, not Base64 or binary vectors.
 */

/**
 * Channel configuration values.
 */
struct Channel
{
    std::string name;
    std::string key;
};

/**
 * User identity values.
 */
struct User
{
    std::string name;
    std::string publicKey;
    std::string privateKey;
};

/**
 * Friend identity values.
 */
struct Friend{
    std::string name;
    std::string publicKey; 
};

namespace Config
{
    inline const Channel PublicChannel{
        "MediumFast",
        "d4f1bb3a20290759f0bcffabcf4e6901"
    };

    inline const Channel PrivateChannel{
        "temp_sal",
        "735066526841576c355055494b723855"
    };


    inline const Friend Admn{
        "f9b38efc",
        "2d72f07c5f1e83195807cedb64b26fd713ea215b5da89c940f51c09d0c5e4954",
        
    }; 

    #ifdef TESTS
    //TEST ONLY!! DON'T TOUCH THIS TO MODIFY THE DEVICE'S CREDENTIALS!
    inline const User Device{
        "12341234",
        "f355b53990d016e7c4722aaa8790f84bd45494723a3be3689c93978da1924a08",
        "c80207fa7a245842762ccc395bd356457688e05d2f9c1466323c7684b5a28b5c"
    };
    #else
    inline const User Device{
        "11111112",
        "64318262105c260bb20a3047b739f32f066cf140ea9b85ae772042a571eef40a",
        "782b135f817fc4b37daf68301919e14da33ec821e79ebe048926e153378fcb5d"
    };
    #endif

    inline const User Leo1{
        "f9b38efc",
        "5886a580f2fa72aa0e17d3fbd2e5acd804ae8915aa2718030dbd0a449aca247e",
        "b0fdb0e27f48ad404ee7fe340ae81322ec0bda50371268b838f2a0e22dabe75d"
    };

}











#endif