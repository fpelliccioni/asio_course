#include <iostream>
#include <string>
#include <string_view>

#include <asio.hpp>
#include <asio/experimental/as_tuple.hpp>

#include <fmt/core.h>
#include <fmt/ostream.h>
#include <fmt/std.h>

#include <nlohmann/json.hpp>

using asio::awaitable;
using asio::co_spawn;
using asio::detached;
using asio::ip::tcp;
using json = nlohmann::json;

constexpr auto use_nothrow_awaitable = asio::experimental::as_tuple(asio::use_awaitable);

constexpr char const* user_agent = "curl/7.87.0";

std::string base64_encode(std::string_view in) {
    constexpr char const* base64_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string out;

    int val = 0;
    int valb = -6;
    for (auto c : in) {
        val = (val<<8) + c;
        valb += 8;
        while (valb>=0) {
            out.push_back(base64_chars[(val >> valb) & 0x3F]);
            valb -= 6;
        }
    }
    if (valb >- 6) out.push_back(base64_chars[((val <<8 ) >> (valb + 8)) & 0x3F]);
    while (out.size()%4) out.push_back('=');
    return out;
}

awaitable<void> rpc_call(tcp::endpoint endpoint, std::string const& credentials, json const& request_json) {
    tcp::socket socket(co_await asio::this_coro::executor);

    // Connect
    {
        auto [e] = co_await socket.async_connect(endpoint, use_nothrow_awaitable);
        if (e) {
            fmt::print(stderr, "Error connecting: {}\n", e.message());
            co_return;
        }
    }

    // Send request
    {
        std::string request = request_json.dump();

        auto const http_request = fmt::format(
            "POST / HTTP/1.1\r\n"
            "Host: {}:{}\r\n"
            "Authorization: Basic {}\r\n"
            "User-Agent: {}\r\n"
            "Accept: */*\r\n"
            "content-type: text/plain;\r\n"
            "Content-Length: {}\r\n\r\n"
            "{}",
            endpoint.address().to_string(),
            endpoint.port(),
            base64_encode(credentials),
            user_agent,
            request.size(),
            request
        );

        auto const [e, _] = co_await asio::async_write(socket, asio::buffer(http_request), use_nothrow_awaitable);
        if (e) {
            fmt::print(stderr, "Error sending request: {}\n", e.message());
            co_return;
        }
    }

    asio::streambuf response_buf;

    // Read status line
    {
        auto const [e, n] = co_await asio::async_read_until(socket, response_buf, "\r\n", use_nothrow_awaitable);
        if (e) {
            fmt::print(stderr, "Error reading the response status line: {}\n", e.message());
            co_return;
        }

        std::string status_line(asio::buffers_begin(response_buf.data()), asio::buffers_begin(response_buf.data()) + n);
        response_buf.consume(n);

        if (status_line.compare(0, 5, "HTTP/") != 0) {
            fmt::print(stderr, "Invalid response\n");
            co_return;
        }

        size_t space_pos1 = status_line.find(' ');
        size_t space_pos2 = status_line.find(' ', space_pos1 + 1);

        if (space_pos1 == std::string::npos || space_pos2 == std::string::npos) {
            fmt::print(stderr, "Invalid response status line\n");
            co_return;
        }

        int status_code = 0;
        auto const r1 = std::from_chars(status_line.data() + space_pos1 + 1, status_line.data() + space_pos2, status_code);

        if (r1.ec != std::errc{}) {
            fmt::print(stderr, "Failed to parse status code\n");
            co_return;
        }

        if (status_code != 200) {
            fmt::print(stderr, "Response returned with status code {}\n", status_code);
            co_return;
        }
    }

    std::size_t content_length = 0;
    // Read headers
    {
        auto const [e, n3] = co_await asio::async_read_until(socket, response_buf, "\r\n\r\n", use_nothrow_awaitable);
        if (e) {
            fmt::print(stderr, "Error reading response headers: {}\n", e.message());
            co_return;
        }

        std::string headers_str(
            asio::buffers_begin(response_buf.data()),
            asio::buffers_begin(response_buf.data()) + n3);
        response_buf.consume(n3);

        std::size_t content_length_pos = headers_str.find("Content-Length: ");
        if (content_length_pos == std::string::npos) {
            fmt::print(stderr, "Content-Length header not found\n");
            co_return;
        }

        std::size_t content_length_end_pos = headers_str.find("\r\n", content_length_pos);
        if (content_length_end_pos == std::string::npos) {
            fmt::print(stderr, "Content-Length line is malformed\n");
            co_return;
        }

        auto const r2 = std::from_chars(
            headers_str.data() + content_length_pos + 16,
            headers_str.data() + content_length_end_pos,
            content_length
        );

        if (r2.ec != std::errc{}) {
            fmt::print(stderr, "Failed to parse Content-Length\n");
            co_return;
        }
    }

    while (response_buf.size() < content_length) {
        auto const [e, _] = co_await asio::async_read(socket, response_buf, use_nothrow_awaitable);
        if (e == asio::error::eof) {
            break;
        }

        if (e) {
            fmt::print(stderr, "Error reading response body: {}\n", e.message());
            co_return;
        }
    }

    if (response_buf.size() > 0) {
        std::cout << &response_buf;
    }
}

int main(int argc, char* argv[]) {
    try {
        if (argc != 3) {
            fmt::print(stderr, "Usage: jsonrpc_client <server_ip> <server_port>\n");
            return 1;
        }

        asio::io_context ctx;

        auto endpoint = *tcp::resolver(ctx).resolve(argv[1], argv[2]);

        json request = {
            {"jsonrpc", "1.0"},
            {"id", "test"},
            {"method", "getblockhash"},
            {"params", {0}}
        };

        std::string const credentials = "bitcoin:bitcoin";

        co_spawn(ctx, rpc_call(endpoint, credentials, request), detached);

        ctx.run();
    } catch (std::exception const& e) {
        fmt::print(stderr, "Exception: {}\n", e.what());
    }
}
