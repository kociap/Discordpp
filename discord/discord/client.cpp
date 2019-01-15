#include "client.hpp"

#include "exception.hpp"
#include "opcodes.hpp"
#include "urls.hpp"

#include "nlohmann/json.hpp"
#include "rpp/rpp.hpp"
#include "websocketpp/connection.hpp"

#include <map>

// Timer
namespace discord {
    Timer::Timer(Stop_Function const& stop) : stop(stop) {}

    void Timer::cancel() {
        stop();
    }
} // namespace discord

// Client
namespace discord {
    static void handle_request_errors(rpp::Response const& res) {
        if (res.status >= 400) {
            throw Http_Request_Failed(res.status, res.text);
        }
    }

    Client::Client(String const& token) : gateway(), token(token), heartbeat_timer([]() {}) {}
    Client::Client(String&& token) : gateway(), token(std::move(token)), heartbeat_timer([]() {}) {}

    Client::~Client() {}

    static void handle_request_error(rpp::Response const& res) {
        if (res.status >= 400) {
            throw Http_Request_Failed(res.status, res.text);
        }
    }

    User Client::get_me() {
        rpp::Headers headers({{"Authorization", token}});
        rpp::Request req;
        req.set_headers(headers);
        //        req.set_verbose(true);
        req.set_verify_ssl(false);
        rpp::Response res = req.get(url::me);
        nlohmann::json json = nlohmann::json::parse(res.text);
        return User::from_json(json);
    }

    Guilds Client::get_guilds() {
        rpp::Headers headers({{"Authorization", token}});
        rpp::Request req;
        req.set_headers(headers);
        //        req.set_verbose(true);
        req.set_verify_ssl(false);
        rpp::Response res = req.get(url::my_guilds);
        nlohmann::json json = nlohmann::json::parse(res.text);
        Guilds guilds = json;
        return guilds;
    }

    Relationships Client::get_relationships() {
        rpp::Headers headers({{"Authorization", token}});
        rpp::Request req;
        req.set_headers(headers);
        req.set_verify_ssl(false);
        rpp::Response res = req.get(url::relationships);
        nlohmann::json json = nlohmann::json::parse(res.text);
        Relationships rels = json;
        return rels;
    }

    Channel Client::create_dm(Users const& recipients) {
        std::string array_elements;
        for (auto& user : recipients) {
            if (array_elements.empty()) {
                array_elements.append(user.id);
            } else {
                array_elements.append("," + user.id);
            }
        }

        rpp::Body body("{\"recipients\":[" + array_elements + "]}");
        rpp::Headers headers({{"Authorization", token}, {"Content-Type", "application/json"}});
        rpp::Request req;
        req.set_headers(headers);
        req.set_verify_ssl(false);
        rpp::URL url = url::create_dm(current_user.id);
        rpp::Response res = req.post(url, body);
        handle_request_errors(res);
        nlohmann::json json = nlohmann::json::parse(res.text);
        return Channel::from_json(json);
    }

    /*User Client::get_user(Snowflake const& user_id) {
        return User();
    }*/

    Channels Client::get_guild_channels(Snowflake const& guild_id) {
        rpp::Headers headers({{"Authorization", token}});
        rpp::Request req;
        req.set_headers(headers);
        //        req.set_verbose(true);
        req.set_verify_ssl(false);
        rpp::Response res = req.get(url::base + "/guilds/" + guild_id + "/channels");
        nlohmann::json json = nlohmann::json::parse(res.text);
        Channels channels = json;
        return channels;
    }

    Image Client::get_avatar(User const& user, uint32_t size) {
        Image image;
        String image_extension;
        rpp::URL avatars_url;
        if (user.avatar) {
            // Check extension. If hash begins with "a_", it's available as gif
            if (user.avatar.value()[0] == 'a' && user.avatar.value()[1] == '_') {
                image_extension = "gif";
                image.format = Image_Format::gif;
            } else {
                image_extension = "png";
                image.format = Image_Format::png;
            }

            avatars_url = url::avatar(user.id, user.avatar.value(), image_extension, size);
        } else {
            // Request default avatar
            avatars_url = url::default_avatar(user.discriminator, size);
        }
        rpp::Request req;
        req.set_verify_ssl(false);
        rpp::Response res = req.get(avatars_url);
        // TODO add more error handling
        handle_request_errors(res);
        image.data = std::move(res.text);
        image.width = size;
        image.height = size;
        return image;
    }

    Image Client::get_guild_icon(Guild const& guild, uint32_t size) {
        Image image;
        image.format = Image_Format::png;
        if (guild.icon) {
            rpp::URL icon_url = url::guild_icon(guild.id, guild.icon.value(), "png", size);
            rpp::Request req;
            req.set_verify_ssl(false);
            rpp::Response res = req.get(icon_url);
            // TODO add more error handling
            handle_request_errors(res);
            image.data = std::move(res.text);
            image.width = size;
            image.height = size;
        }
        return image;
    }

    void Client::send_message(Snowflake const& channel_id, String const& message) {
        rpp::Body body;
        body.append({{"content", message}});
        rpp::Headers headers({{"Authorization", token}});
        rpp::Request req;
        req.set_headers(headers);
        //        req.set_verbose(true);
        req.set_verify_ssl(false);
        rpp::Response res = req.post(url::base + "/channels/" + channel_id + "/messages", body);
    }

    void Client::connect(String const& url) {
        // Debug logging
        websocket.clear_access_channels(websocketpp::log::alevel::all);
        websocket.set_access_channels(websocketpp::log::alevel::connect);
        websocket.set_access_channels(websocketpp::log::alevel::disconnect);

        websocket.set_tls_init_handler([](websocketpp::connection_hdl) {
            return std::make_shared<websocketpp::lib::asio::ssl::context>(websocketpp::lib::asio::ssl::context::tlsv13_client);
        });

        websocket.init_asio();
        websocket.set_message_handler([this](websocketpp::connection_hdl handle, Websocket::message_ptr msg) -> void {
            websocket_message_handler(handle, msg); // force clang to format this line properly
        });

        websocketpp::lib::error_code code;
        Websocket::connection_ptr connection = websocket.get_connection(url, code);
        if (code) {
            on_websocket_error(Websocket_Error::connection_failed, code.message());
            // Let outside know it's not connected maybe?
            return;
        }

        handle = connection->get_handle();
        websocket.connect(connection);
        thread.reset(new std::thread([this]() { websocket.run(); }));
        on_connect();
    }

    void Client::disconnect(uint32_t code, String const& reason) {
        heartbeat_timer.cancel();
        websocket.close(handle, code, reason);
        on_disconnect();
        if (thread) {
            thread->join();
        }
    }

    void Client::websocket_message_handler(websocketpp::connection_hdl handle, Websocket::message_ptr msg) {
        nlohmann::json parsed_msg = nlohmann::json::parse(msg->get_payload());
        int opcode = (parsed_msg.count("op") == 1 ? parsed_msg.at("op").get<int>() : -1);
        if (opcode == opcodes::gateway::hello) {
            heartbeat_interval = parsed_msg.at("d").at("heartbeat_interval").get<uint32_t>();
            identify();
            heartbeat();
        } else if (opcode == opcodes::gateway::heartbeat_ack) {
            heartbeat_ack = true;
        } else if (opcode == opcodes::gateway::dispatch) {
            String type = parsed_msg.at("t").get<String>();
            if (type == "READY") {
                // TODO add presences, read_state, guilds, notes (???), guild_experiments (???), friends_suggestion_count,
                //      experiments(???), consents, connected_accounts, analytics_token, _trace
                auto data = parsed_msg.at("d");
                User_Settings user_settings = data.at("user_settings");
                User user = data.at("user");
                Relationships relationships = data.at("relationships");
                Guilds guilds = data.at("guilds");
                Channels dms = data.at("private_channels");
                current_user = user;
                on_ready(user_settings, user, relationships, dms, guilds);
            } else if (type == "PRESENCE_UPDATE") {
            } else if (type == "TYPING_START") {
            } else if (type == "MESSAGE_CREATE") {
                on_message(Message::from_json(parsed_msg.at("d")));
            }
        } else {
            // Unknown opcode
            on_websocket_message(handle, msg);
        }
    }

    void Client::websocket_send(websocketpp::connection_hdl handle, String const& payload) {
        websocketpp::lib::error_code ec;
        websocket.send(handle, payload, websocketpp::frame::opcode::text, ec);
        // TODO add error handling
    }

    Timer Client::set_timer(uint32_t time, std::function<void()> callback) {
        Websocket::timer_ptr timer = websocket.set_timer(time, [callback](websocketpp::lib::error_code const& code) {
            if (code == websocketpp::transport::error::operation_aborted) {
                return;
            }

            callback();
        });

        return Timer([timer]() { timer->cancel(); });
    }

    void Client::identify() {
        websocket_send(handle, "{\"op\":2,\"d\":{\"token\":\"" + token + "\",\"properties\":{\"os\":\"Windows\"}}}");
    }

    void Client::heartbeat() {
        if (!heartbeat_ack) {
            // TODO
            // No ack, terminate connection
        }

        heartbeat_ack = false;
        websocket_send(handle, "{\"op\":1,\"d\":null}");
        on_heartbeat();
        heartbeat_timer = set_timer(heartbeat_interval, [this]() { heartbeat(); });
    }

    // Discord events
    void Client::on_message(Message const&) {}
    void Client::on_reaction() {}
    void Client::on_presence_update() {}

    // Websocket events
    void Client::on_heartbeat() {}
    void Client::on_connect() {}
    void Client::on_ready(User_Settings const&, User const&, Relationships const&, Channels const&, Guilds const&) {}
    void Client::on_disconnect() {}
    void Client::on_websocket_error(Websocket_Error error, String const& message) {}
    void Client::on_websocket_message(websocketpp::connection_hdl handle, websocketpp::connection<websocketpp::config::asio_tls_client>::message_ptr msg) {}

} // namespace discord