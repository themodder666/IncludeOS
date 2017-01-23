#include <net/tcp/connection.hpp>
namespace net{
    namespace tcp{
                Connection::Translator::Translator(Connection_ptr _ptr):ptr(_ptr){
                    ptr->setup_default_callbacks();

                }
                void Connection::Translator::on_connect(){
                    ptr->on_connect_(ptr);
                };
                void Connection::Translator::on_read( buffer_t buf, size_t sz){
                    ptr->read_request.callback(buf, sz);
                };
                void Connection::Translator::on_disconnect(Connection::Disconnect a ){
                    ptr->on_disconnect_(ptr,a);
                }
                void Connection::Translator::on_error(TCPException ex){
                    ptr->on_error_(ex);
                }
                void Connection::Translator::on_drop(const Packet& pk, const std::string& st){
                    ptr->on_packet_dropped_(pk, st);
                }
                void Connection::Translator::on_close(){
                    ptr->on_close_();
                }
                void Connection::Translator::on_timeout(size_t attempts, double rto){
                    ptr->on_rtx_timeout_(attempts,rto);
                }
                //NOOP for now, ill get back to it
                void Connection::Translator::on_write(WriteBuffer&& buffer, Connection::WriteCallback callback){
                    ptr->writeNew(std::forward<WriteBuffer>(buffer), callback);
                }
                Connection::ConnectCallback Connection::Translator::connectCb(){
                    return ptr->on_connect_;
                }
                Connection::ReadCallback Connection::Translator::readCb(){
                    return ptr->read_request.callback;
                }
                Connection::DisconnectCallback Connection::Translator::disconnectCb(){
                    return ptr->on_disconnect_;
                }
                Connection::CloseCallback Connection::Translator::closeCb(){
                    return ptr->on_close_;
                }
                Connection::ErrorCallback Connection::Translator::errorCb(){
                    return ptr->on_error_;
                }
                Connection::PacketDroppedCallback Connection::Translator::dropCb(){
                    return ptr->on_packet_dropped_;
                }
                Connection::RtxTimeoutCallback Connection::Translator::timeoutCb(){
                    return ptr->on_rtx_timeout_;
                }
                delegate<void(WriteBuffer&&,Connection::WriteCallback)> Connection::Translator::writeCb(){
                    return {ptr.get(),&Connection::writeNew};
                }
    }
}
