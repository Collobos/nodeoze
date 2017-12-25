#ifndef __CPP11OM_SEMAPHORE_H__
#define __CPP11OM_SEMAPHORE_H__

#include <cassert>
#include <atomic>

#if defined( WIN32 )

#	include <windows.h>
#	undef min
#	undef max

#elif defined( __MACH__ )

#	include <mach/mach.h>

#elif defined( __unix__ )

#	include <semaphore.h>

#endif

namespace nodeoze {

#if defined(_WIN32)

class semaphore
{
public:

	semaphore( int count = 0)
    {
		assert( count >= 0);
		m_native = CreateSemaphore( NULL, count, MAXLONG, NULL );
    }

	~semaphore()
    {
        CloseHandle( m_native );
    }

	inline void
	wait()
    {
        WaitForSingleObject( m_native, INFINITE );
    }

	void
	signal( int count = 1 )
    {
        ReleaseSemaphore( m_native, count, nullptr );
    }

private:

	semaphore(const semaphore& other) = delete;

	semaphore&
	operator=(const semaphore& other) = delete;

    HANDLE m_native;
};

#elif defined( __MACH__ )

class semaphore
{
public:

    semaphore( int count = 0 )
    {
        assert( count >= 0 );
        semaphore_create( mach_task_self(), &m_native, SYNC_POLICY_FIFO, count );
    }

	~semaphore()
    {
        semaphore_destroy( mach_task_self(), m_native );
    }

	inline void
	wait()
    {
        semaphore_wait( m_native );
    }

	inline void
	signal()
    {
		semaphore_signal( m_native );
    }

	inline void
	signal( int count )
    {
		while (count-- > 0 )
		{
			semaphore_signal( m_native );
        }
	}

private:

    semaphore_t m_native;

	semaphore( const semaphore& other) = delete;

	semaphore&
	operator=( const semaphore& other) = delete;
};

#elif defined( __unix__ )

class semaphore
{
public:

	semaphore( int count = 0 )
    {
		assert( count >= 0 );
		sem_init( &m_native, 0, count );
    }

	~semaphore()
    {
        sem_destroy( &m_native );
    }

    void wait()
    {
        // http://stackoverflow.com/questions/2013181/gdb-causes-sem-wait-to-fail-with-eintr-error
        int rc;
        do
        {
            rc = sem_wait(&m_native);
        }
        while (rc == -1 && errno == EINTR);
    }

    void signal()
    {
        sem_post(&m_native);
    }

    void signal(int count)
    {
        while (count-- > 0)
        {
            sem_post(&m_native);
        }
    }

private:

	semaphore(const semaphore& other) = delete;

	semaphore&
	operator=(const semaphore& other) = delete;

	sem_t m_native;
};

#endif

}


#endif
