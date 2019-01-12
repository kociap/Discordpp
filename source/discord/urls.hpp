#ifndef DISCORD_URLS_HPP
#define DISCORD_URLS_HPP

#include "rpp/url.hpp"
#include "types.hpp"

namespace discord {
    namespace url {
        // TODO change type from String to URL

        // POST
        // Object with 3 optional fields
        // {
        //   email : [],
        //   password: [],
        //   captcha: []
        // }
        // Arrays contain error messages
        String const login("https://discordapp.com/api/auth/login");

        String const base("https://discordapp.com/api");

        // GET
        String const me("https://discordapp.com/api/users/@me");

        // GET
        String const my_guilds("https://discordapp.com/api/users/@me/guilds");

        // GET
        // Array of Relationship objects
        // [ { type, id, user { username, discriminator, id, avatar } } ]
        //
        // type:
        //     1 - friends
        String const relationships("https://discordapp.com/api/v6/users/@me/relationships");

        // GET
        String const gateway("https://discordapp.com/api/gateway");

        // CDN endpoints
        // https://discordapp.com/developers/docs/reference#image-formatting-cdn-endpoints

        // GET
        // Avatar image endpoint
        // Supported file formats: PNG, JPEG, GIF
        //
        // URL:
        //     https://cdn.discordapp.com/avatars/user_id/avatar_hash.image_extension
        //
        // Query params:
        //     size - size of the image, may be any power of 2 within [16, 2048]
        rpp::URL avatar(std::string const& user_id, std::string const& avatar_hash, std::string const& image_extension);

        // GET
        // Default avatar image endpoint
        // Supported file formats: PNG
        //
        // URL:
        //     https://cdn.discordapp.com/embed/avatars/modified_discriminator.png
        //
        // Query params:
        //     size - size of the image, may be any power of 2 within [16, 2048]
        rpp::URL default_avatar(std::string const& discriminator);

        // GET
        // Supported file formats: PNG, JPEG
        //
        // URL:
        // 	   https://cdn.discordapp.com/icons/guild_id/icon_hash.image_extension
        //
        // Query params:
        //     size - size of the image, may be any power of 2 within [16, 2048]
        rpp::URL guild_icon(std::string const& guild_id, std::string const& icon_hash, std::string const& image_extension);
    } // namespace url
} // namespace discord
#endif // !DISCORD_URLS_HPP
