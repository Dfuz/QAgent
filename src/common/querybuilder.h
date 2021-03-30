#ifndef QUERYBUILDER_H
#define QUERYBUILDER_H

#include <QTcpSocket>
#include <QDataStream>
#include <QBuffer>
#include <algorithm>
#include <memory>
#include <optional>
#include <type_traits>
#include <variant>
#include <functional>

#include "messagebuilder.h"

namespace Utils {

enum QueryDirection {
    Bidirectional,
    Unidirectional
};

template<MessageType ret>
using Verificator = std::function<bool(const Message<ret> &)>;

template<QueryDirection direct, bool iSsending, MessageType to = NoMessage, MessageType ret = NoMessage>
struct Query {

    template<MessageType invRet>
    using InvokeReturn = std::optional<Message<invRet>>;

    Query(std::shared_ptr<QTcpSocket> _socket):
        socket(_socket)
    {}
    Query(std::shared_ptr<QTcpSocket> _socket, const Message<to> _msg):
        socket(_socket), msg(_msg)
    {}

    /*!
     * \brief toSend - задаем сообщение для отправки
     * \param _msg - сообщение Message<newTo>
     * \param compressionLevel - уровень компресии 0-9, -1 - zlib
     * \return
     */
    template<MessageType newTo>
    constexpr Query<direct, iSsending, newTo, ret> toSend(const Message<newTo> & _msg, int newCompressionLevel = 0) noexcept {
        Query<direct, iSsending, newTo, ret> retvar{socket, _msg};
        return retvar.setCompression(newCompressionLevel);
    };

    /*!
     * \brief toSend - задаем сообщение которое ожидаем
     * \param _msg - сообщение Message<newRet>
     * \param uncompression - нужно ли декомпрессировать
     * \return
     */
    template<MessageType newRet>
    constexpr Query<direct, iSsending, to, newRet> toGet() noexcept {
        return {socket, msg};
    };

    /*!
     * \brief invoke - вызов сессии связи (запись)
     * \return
     */
    template<QueryDirection T = direct,
             std::enable_if_t<
                T == Bidirectional &&
                iSsending,
             bool> = true>
    InvokeReturn<ret> invoke() noexcept {

        writeMessage();

        return readMessage();
    }

    /*!
     * \brief invoke - вызов сессии связи (чтение)
     * \return
     */
    template<QueryDirection T = direct,
             std::enable_if_t<
                T == Bidirectional &&
                iSsending == false,
             bool> = true>
    InvokeReturn<ret> invoke() noexcept {

        auto got = readMessage();

        writeMessage();

        if (!got.has_value())
            return std::nullopt;

        return got;
    }

    /*!
     * \brief invoke - вызов сессии связи
     * (Вариант для Unidirectional) (запись)
     * \return
     */
    template<QueryDirection T = direct,
             std::enable_if_t<
                T == Unidirectional &&
                iSsending,
             bool> = true>
    Message<NoMessage> invoke() noexcept {

        writeMessage();
        return {};
    }

    /*!
     * \brief invoke - вызов сессии связи
     * (Вариант для Unidirectional) (чтение)
     * \return
     */
    template<QueryDirection T = direct,
             std::enable_if_t<
                T == Unidirectional &&
                iSsending == false,
             bool> = true>
    InvokeReturn<ret> invoke() noexcept {

        return readMessage();
    }

    /*!
     * \brief setVerificator
     * \param verFn - верификатор по которому можно проверить сообщение
     * \return
     */
    constexpr Query& setVerificator(Verificator<ret> verFn) noexcept {
        verificators.push_back(verFn);
        return *this;
    }
    /*!
     * \brief setCompression - задаем уровень компрессии
     * \param newLevel
     * \return
     */
    constexpr Query& setCompression(int newLevel) noexcept {
        compressionLevel = newLevel;
        return *this;
    }

    friend class fcmanager_tests;

private:
    std::shared_ptr<QTcpSocket> socket;
    Message<to> msg;
    std::vector<Verificator<ret>> verificators;
    int compressionLevel = 0;

    bool writeMessage()
    {
        // Участок отправки сообщения
        if constexpr (to != NoMessage) {
            auto toSend = qCompress(msg.toJson(), compressionLevel);
            quint16 size = toSend.size();
            socket->write(reinterpret_cast<const char*>(&size), sizeof(quint16));
            socket->write(toSend);
            if (!socket->waitForBytesWritten()) {
                qDebug()<<"Query: failed to write";
                return false;
            }
            qDebug()<<"Query: sended "<<toSend<<"\nbytes "<<toSend.size();
        }
        return true;
    }

    std::optional<Message<ret>> readMessage()
    {
        // Участок приема сообщения
        if constexpr (ret == NoMessage)
            return Message<NoMessage>{};

        qDebug()<<"Query: waiting to read";

        if (!socket->waitForReadyRead())
            return std::nullopt;

        auto gotSize = socket->read(sizeof(quint16));
        auto gotRaw = socket->read(reinterpret_cast<const quint16*>(gotSize.constData())[0]);
        qDebug()<<"Query: readed "<<gotRaw;

        auto got = Message<ret>::parseJson(qUncompress(gotRaw));

        if(!got.has_value()) {
            qDebug()<<"Query: failed to parse: "<<gotRaw.data();
            return std::nullopt;
        }
        
        if (!checkVerificators(got.value()))
            return std::nullopt;

        return got;
    }

    bool checkVerificators(const Message<ret> &msg)
    {
        if (verificators.empty()) return true;
        return std::all_of(verificators.cbegin(), verificators.cend(),
                        [&](auto fn) { return fn(msg);});
    }
};

struct QueryBuilder {
public:
    QueryBuilder() {}
    QueryBuilder(std::shared_ptr<QTcpSocket> skt): socket(skt) {}
    QueryBuilder(QTcpSocket *skt): socket(std::move(skt)) {}
    QueryBuilder(qintptr ptrskt)
    {
        if (!socket->setSocketDescriptor(ptrskt))
        {
            throw std::runtime_error("setDescriptor failed");
        }
    }

    bool setSocketDescriptor(qintptr ptrskt) {
        return socket->setSocketDescriptor(ptrskt);
    }

    /*!
     * \brief makeQuery - создание сесси связи
     * \return
     */
    template<QueryDirection direct = Bidirectional>
    Query<direct, true> makeQuery() noexcept {
        return {socket};
    };

    /*!
     * \brief makeQuery - создание сесси связи
     * \return
     */
    template<QueryDirection direct = Bidirectional>
    Query<direct, false> makeQueryRead() noexcept {
        return {socket};
    };

    /*!
     * \brief onlySend - вариант только отправки
     * \param msg
     * \return
     */
    template<MessageType to>
    Query<Unidirectional, true, to, NoMessage> onlySend(const Message<to> & msg) noexcept {
        return makeQuery<Unidirectional>()
                .toGet<NoMessage>()
                .toSend<to>(msg);
    }

    /*!
     * \brief onlyGet - вариант только получения
     * \return
     */
    template<MessageType ret>
    Query<Bidirectional, false, NoMessage, ret> onlyGet() noexcept {
        return makeQueryRead<Bidirectional>()
                .toSend<NoMessage>({})
                .toGet<ret>();
    }

    std::shared_ptr<QTcpSocket> socket = std::make_shared<QTcpSocket>();
};

}

#endif // QUERYBUILDER_H
