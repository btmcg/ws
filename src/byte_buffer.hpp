#pragma once

#include <cstdint>
#include <cstring> // std::memmove
#include <utility> // std::exchange


template <std::size_t Capacity>
class byte_buffer
{
private:
    std::uint8_t* buf_ = nullptr;
    std::uint8_t const* rptr_ = nullptr;
    std::uint8_t* wptr_ = nullptr;

public:
    byte_buffer() noexcept;
    ~byte_buffer() noexcept;

    // prevent copy operations
    byte_buffer(byte_buffer const&) noexcept = delete;
    byte_buffer& operator=(byte_buffer const&) noexcept = delete;

    // enable move operations
    byte_buffer(byte_buffer&&) noexcept;
    byte_buffer& operator=(byte_buffer&&) noexcept;

    std::uint8_t const* read_ptr() const noexcept;
    std::uint8_t* write_ptr() noexcept;

    void bytes_read(std::size_t) noexcept;
    void bytes_written(std::size_t) noexcept;
    std::size_t shift() noexcept;

    std::size_t capacity() const noexcept;
    std::size_t bytes_unread() const noexcept;
    std::size_t bytes_left() const noexcept;
};


/**********************************************************************/

template <std::size_t Capacity>
byte_buffer<Capacity>::byte_buffer() noexcept
        : buf_(new std::uint8_t[Capacity])
        , rptr_(buf_)
        , wptr_(buf_)
{
    // empty
}

template <std::size_t Capacity>
byte_buffer<Capacity>::~byte_buffer() noexcept
{
    delete[] buf_;
}

template <std::size_t Capacity>
byte_buffer<Capacity>::byte_buffer(byte_buffer&& rhs) noexcept
        : buf_(std::exchange(rhs.buf_, nullptr))
        , rptr_(std::exchange(rhs.rptr_, nullptr))
        , wptr_(std::exchange(rhs.wptr_, nullptr))
{
    // empty
}

template <std::size_t Capacity>
byte_buffer<Capacity>&
byte_buffer<Capacity>::operator=(byte_buffer&& rhs) noexcept
{
    if (this != &rhs) {
        delete[] buf_;
        buf_ = std::exchange(rhs.buf_, nullptr);
        rptr_ = std::exchange(rhs.rptr_, nullptr);
        wptr_ = std::exchange(rhs.wptr_, nullptr);
    }
    return *this;
}

template <std::size_t Capacity>
std::uint8_t const*
byte_buffer<Capacity>::read_ptr() const noexcept
{
    return rptr_;
}

template <std::size_t Capacity>
std::uint8_t*
byte_buffer<Capacity>::write_ptr() noexcept
{
    return wptr_;
}

template <std::size_t Capacity>
void
byte_buffer<Capacity>::bytes_read(std::size_t nbytes) noexcept
{
    rptr_ += nbytes;
}

template <std::size_t Capacity>
void
byte_buffer<Capacity>::bytes_written(std::size_t nbytes) noexcept
{
    wptr_ += nbytes;
}

template <std::size_t Capacity>
std::size_t
byte_buffer<Capacity>::shift() noexcept
{
    std::size_t const unread = bytes_unread();

    // if there are no bytes unread, then there's nothing to shift and
    // we can cheat and just reset everything to beginning of the buffer
    if (unread == 0) {
        rptr_ = wptr_ = buf_;
        return 0;
    }

    std::memmove(buf_, rptr_, unread);
    rptr_ = buf_;
    wptr_ = buf_ + unread;
    return unread;
}

template <std::size_t Capacity>
std::size_t
byte_buffer<Capacity>::capacity() const noexcept
{
    return Capacity;
}

template <std::size_t Capacity>
std::size_t
byte_buffer<Capacity>::bytes_unread() const noexcept
{
    return wptr_ - rptr_;
}

template <std::size_t Capacity>
std::size_t
byte_buffer<Capacity>::bytes_left() const noexcept
{
    return (buf_ + Capacity) - wptr_;
}
