#ifndef NODEOZE_BSTREAM_IBFILEBUF_H
#define NODEOZE_BSTREAM_IBFILEBUF_H

#include <nodeoze/bstream/ibstreambuf.h>

#ifndef NODEOZE_BSTREAM_DEFAULT_IBFILEBUF_SIZE
#define NODEOZE_BSTREAM_DEFAULT_IBFILEBUF_SIZE  16384UL
#endif

namespace nodeoze 
{
namespace bstream 
{

class ibfilebuf : public ibstreambuf
{
public:

    ibfilebuf( size_type buffer_size = NODEOZE_BSTREAM_DEFAULT_IBFILEBUF_SIZE );

    ibfilebuf( std::string const& filename, std::error_code& err, int flag_overrides = 0, size_type buffer_size = NODEOZE_BSTREAM_DEFAULT_IBFILEBUF_SIZE );

    ibfilebuf( std::string const& filename, int flag_overrides = 0, size_type buffer_size = NODEOZE_BSTREAM_DEFAULT_IBFILEBUF_SIZE );

    void
    open( std::string const& filename, std::error_code& err, int flag_overrides = 0 );

    void
    open( std::string const& filename, int flag_overrides = 0 );

    bool
    is_open() const noexcept
    {
        return m_is_open;
    }
    
    void
    close( std::error_code& err );

    void
    close();

protected:

    virtual position_type
    really_seek( seek_anchor where, offset_type offset, std::error_code& err ) override;

    virtual position_type
    really_tell( seek_anchor where, std::error_code& err ) override;

    virtual size_type
    really_underflow( std::error_code& err ) override;

private:

    size_type
    load_buffer( std::error_code& err );

    void
    reset_ptrs()
    {
        auto base = m_buf.data();
        set_ptrs( base, base, base );
    }

    void 
    really_open( std::error_code& err );

    buffer              m_buf;
    std::string         m_filename;
    bool                m_is_open;
    int                 m_flags;
    int                 m_fd;
};

} // namespace bstream
} // namespace nodeoze

#endif // NODEOZE_BSTREAM_IBFILEBUF_H