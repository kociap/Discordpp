#ifndef DISCORD_CLIENT_HPP
#define DISCORD_CLIENT_HPP

#include "classes/channel.hpp"
#include "classes/usersettings.hpp"
#include "classes/guild.hpp"
#include "image.hpp"
#include "classes/message.hpp"
#include "classes/relationship.hpp"
#include "types.hpp"
#include "classes/user.hpp"

#ifdef _MSC_VER
// Disable all warnings from websocketpp and asio
#pragma warning(push, 0)
#endif // _MSC_VER

#define _WEBSOCKETPP_CPP11_RANDOM_DEVICE_
#define _WEBSOCKETPP_CPP11_TYPE_TRAITS_
#define ASIO_STANDALONE
#include "websocketpp/client.hpp"
#include "websocketpp/config/asio_client.hpp"

#ifdef _MSC_VER
// Restore warnings
#pragma warning(pop)
#endif // _MSC_VER

#include <thread>
#include <vector>

namespace discord {
    class Timer {
    public:
        using Stop_Function = std::function<void()>;

        Timer(Stop_Function const&);
        void cancel();

    private:
        Stop_Function stop;
    };

    class Client {
    public:
        enum class Websocket_Error {
            connection_failed,
        };

        Client(String const& token);
        Client(String&& token);
        virtual ~Client();

        void connect(String const& url);
        void disconnect(uint32_t code, String const& reason);

        // Current user
        User get_me();
        Guilds get_guilds();
        Relationships get_relationships();
        //Channels get_dms();

        // Create a dm or a group dm
        Channel create_dm(Users const& recipients);

        //User get_user(Snowflake const& user_id);

        Channels get_guild_channels(Snowflake const& guild_id);

        // Requested avatar is size x size px big
        Image get_avatar(User const&, uint32_t size);

        // Requested icon is size x size px big
        Image get_guild_icon(Guild const&, uint32_t size);

        void send_message(Snowflake const& channel_id, String const& message);

        // Discord events
        virtual void on_message(Message const&);
        virtual void on_reaction();
        virtual void on_presence_update();

        // Websocket events
        virtual void on_heartbeat();
        virtual void on_connect();
        virtual void on_ready(User_Settings const&, User const&, Relationships const&, Channels const& dms, Guilds const&);
        virtual void on_disconnect();
        virtual void on_websocket_error(Websocket_Error error, String const& message);
        virtual void on_websocket_message(websocketpp::connection_hdl, websocketpp::connection<websocketpp::config::asio_tls_client>::message_ptr);

    protected:
        using Websocket = websocketpp::client<websocketpp::config::asio_tls_client>;
        Websocket websocket;

        User current_user;

    private:
        String gateway;
        String const token;
        std::shared_ptr<std::thread> thread = nullptr;
        websocketpp::connection_hdl handle;
        Timer heartbeat_timer;
        uint32_t heartbeat_interval = 0;
        bool heartbeat_ack = false;

        Timer set_timer(uint32_t time, std::function<void()> callback);
        void identify();
        void heartbeat();

        void websocket_message_handler(websocketpp::connection_hdl handle, Websocket::message_ptr msg);
        void websocket_send(websocketpp::connection_hdl handle, String const& payload);
    };
} // namespace discord

#endif // !DISCORD_CLIENT_HPP