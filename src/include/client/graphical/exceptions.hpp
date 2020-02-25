#pragma once

#include <stdexcept>
#include <string_view>
#include <fmt/format.h>

namespace familyline::graphics {

    class graphical_exception : public std::runtime_error
    {
    protected:
        std::string_view _message;
        
    public:
        explicit graphical_exception(std::string_view message);

        virtual const char* what() const noexcept {
            return fmt::format("Graphical error {}", this->_message.data()).c_str();
        }
    };


    class renderer_exception : public graphical_exception
    {
    private:
        int errorCode;
        std::string _msg;

    public:
        explicit renderer_exception(std::string_view message, int code)
            : graphical_exception(message), errorCode(code)
            {
                _msg = fmt::format("Renderer error {}, code {}",
                                   this->_message.data(), this->errorCode);
            }
        
        virtual const char* what() const noexcept {
            return _msg.c_str();
        }

    };

    class shader_exception : public graphical_exception
    {
    private:
        int errorCode;
        std::string _msg;

    public:
        explicit shader_exception(std::string_view message, int code)
            : graphical_exception(message), errorCode(code)
            {
                _msg = fmt::format("Shader error {}, code {}",
                                   this->_message.data(), this->errorCode);
            }

        virtual const char* what() const noexcept {
            return _msg.c_str();
        }

    };

    
    /**
     * A list of possible asset errors
     */
    enum class AssetError {
        AssetFileOpenError = 1025,
        AssetFileParserError,
        InvalidAssetType,
        AssetNotFound,
        AssetOpenError
    };

    
    class asset_exception : public graphical_exception
    {
    private:
        AssetError errorCode;
        std::string _msg;
        
    public:
        explicit asset_exception(std::string_view message, AssetError code)
            : graphical_exception(message), errorCode(code)
            {
                _msg = fmt::format("Asset error {}, code {}",
                                   this->_message.data(), (int)this->errorCode);
            }
        
        virtual const char* what() const noexcept {
            return _msg.c_str();
        }
    };
}