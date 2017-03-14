// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2017 Oslo and Akershus University College of Applied Sciences
// and Alfred Bratterud
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <net/http/client_connection.hpp>
#include <net/http/client.hpp>

#include <debug>

namespace http {

  Client_connection::Client_connection(Client& client, Stream_ptr stream)
    : Connection{std::move(stream)},
      client_(client),
      req_(nullptr),
      res_(nullptr),
      on_response_{nullptr},
      timer_({this, &Client_connection::timeout_request}),
      timeout_dur_{timeout_duration::zero()}
  {
    // setup close event
    stream_->on_close({this, &Client_connection::close});
  }

  void Client_connection::send(Request_ptr req, Response_handler on_res, const size_t bufsize, timeout_duration timeout)
  {
    Expects(available());
    req_ = std::move(req);
    on_response_ = std::move(on_res);
    Expects(on_response_ != nullptr);
    timeout_dur_ = timeout;

    if(timeout_dur_ > timeout_duration::zero())
      timer_.restart(timeout_dur_);

    send_request(bufsize);
  }

  void Client_connection::send_request(const size_t bufsize)
  {
    keep_alive_ = (req_->header().value(header::Connection) != "close");

    stream_->on_read(bufsize, {this, &Client_connection::recv_response});

    stream_->write(req_->to_string());
  }

  void Client_connection::recv_response(buffer_t buf, size_t len)
  {
    if(len == 0) {
      end_response({Error::NO_REPLY});
      return;
    }

    const auto data = std::string{(char*)buf.get(), len};

    // restart timer since we got data
    if(timer_.is_running())
      timer_.restart(timeout_dur_);

    // create response if not exist
    if(res_ == nullptr)
    {
      try {
        res_ = make_response(data); // this also parses
      }
      catch(...)
      {
        end_response({Error::INVALID});
        return;
      }
    }
    // if there already is a response
    else
    {
      // this is the case when Status line is received, but not yet headers.
      if(res_->header().is_empty() && req_->method() != HEAD)
      {
        *res_ << data;
        res_->parse();
      }
      // here we assume all headers has already been received (could not be true?)
      else
      {
        // add chunks of body data
        res_->add_chunk(data);
      }
    }

    const auto& header = res_->header();
    // TODO: Temporary, not good enough
    // if(res_->is_complete())
    // Assume we want some headers
    if(!header.is_empty())
    {
      if(header.has_field(header::Content_Length))
      {
        try
        {
          const unsigned conlen = std::stoul(header.value(header::Content_Length).to_string());
          debug2("<http::Connection> [%s] Data: %u ConLen: %u Body:%u\n",
            req_->uri().to_string().to_string().c_str(), data.size(), conlen, res_->body().size());
          // risk buffering forever if no timeout
          if(conlen == res_->body().size())
          {
            end_response();
          }
        }
        catch(...)
        { end_response({Error::INVALID}); }
      }
      else
        end_response();
    }
    else if(req_->method() == HEAD)
    {
      end_response();
    }
  }

  void Client_connection::end_response(Error err)
  {
    // move response to a copy in case of callback result in new request
    Ensures(on_response_);
    auto callback = std::move(on_response_);
    on_response_.reset();

    // stop timeout timer
    timer_.stop();

    callback(err, std::move(res_), *this);
    end();
    /*if(!released())
    {
      // avoid trying to parse any more responses
      tcpconn_->on_read(0, nullptr);

      if(!keep_alive_) // user callback may override this
        shutdown();
    }
    else
    {
      close();
    }*/
  }

  void Client_connection::close()
  {
    // if the user havent received a response yet
    if(on_response_ != nullptr)
    {
      auto callback = std::move(on_response_);
      on_response_.reset();
      timer_.stop();
      callback(Error::CLOSING, std::move(res_), *this);
    }

    client_.close(*this);
  }

}
