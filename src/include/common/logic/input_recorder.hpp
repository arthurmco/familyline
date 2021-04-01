#pragma once

/**
 * Records all received inputs from all clients into a single file, so
 * you can reproduce it later
 *
 * (C) 2020 Arthur Mendes
 */

#include <common/logic/player_actions.hpp>
#include <common/logic/player_manager.hpp>

// This file will be generated when building, so no problem
#include <input_serialize_generated.h>

namespace familyline::logic {

    struct RecordPlayerInfo {
        uint64_t id;
        std::string name;
    };

    class ObjectFactory;
    
    /**
     * The input recorder file is divided into three parts:
     *   - the header, marked by the FREC keyword, 4 bytes contain the version, 
     *     4 bytes with the header length, and flatbuffer data with player information 
     *     and object checksum data.
     *   - the inputs; each input is marked by the FINP keyword. We mark the inputs
     *     because, in the case of an unexpected crash, we can use this file for
     *     debugging; the file will be incomplete, but we can find the input commands
     *     thanks to those keywords.
     *   - the footer, marked by the FEND keyword, contains 4 bytes of keyword, 4 bytes of 
     *     input data count and 4 bytes of checksum
     *
     * We do not store the size of the whole file, because this is more like of a streaming
     * format; it is made to be recoverable and readable even if it is a little bit corrupted, 
     * because of a crash.
     *
     */    
    class InputRecorder {
    public:
        InputRecorder(PlayerManager& pm);

        /**
         * Create the file.
         *
         * Return true if it could create, false if it could not
         */
        bool createFile(std::string_view path, ObjectFactory* const of);


        /**
         * You should pass this function as a callback to the player manager
         * `addListener` function
         */
        bool addAction(PlayerInputAction a);

        void commit();

        ~InputRecorder();

        InputRecorder(InputRecorder&) = delete;
        InputRecorder(const InputRecorder&) = delete;

    private:
        FILE* f_ = nullptr;
        PlayerManager& pm_;
        std::vector<RecordPlayerInfo> pinfo_;

        std::string path_ = "";
        int inputcount_ = 0;

        int pm_handle_ = 0;
    };


}
