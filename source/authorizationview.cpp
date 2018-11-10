#include "authorizationview.hpp"

#include "discord/apilinks.hpp"
#include "rpp/response.hpp"
#include "rpp/rpp.hpp"

#include <QDebug>
#include <QUrlQuery>

AuthorizationView::AuthorizationView(QWidget* parent) : QWebEngineView(parent) {
    connect(this, &AuthorizationView::urlChanged, this, &AuthorizationView::onURLChanged);
}

void AuthorizationView::onURLChanged(QUrl const& url) {
    if (url.toString(QUrl::RemoveQuery) == discord::url::redirect) {
        QUrlQuery query(url);
        if (query.hasQueryItem("code")) {
            rpp::String code = query.queryItemValue("code").toStdString();
            rpp::Body body;
            body.add({rpp::Body_Item{"client_id", discord::auth::clientID.toStdString()},
                      rpp::Body_Item{"client_secret", discord::auth::clientSecret.toStdString()}, rpp::Body_Item{"grant_type", "authorization_code"},
                      rpp::Body_Item{"code", code}, rpp::Body_Item{"redirect_uri", discord::url::redirect.toStdString()},
                      rpp::Body_Item{"scope", discord::auth::scope.toStdString()}});
            rpp::Request req;
            req.set_verify_ssl(false);
            req.set_verbose(true);
            rpp::Response res = req.post(discord::url::token.toStdString(), body);
            qDebug() << "[" << res.status << "] " << QString::fromStdString(res.text);
        } else {
            // Handle errors
        }

        close();
    }
}
