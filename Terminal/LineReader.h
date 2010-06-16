#ifndef __LINE_READER_H_
#define __LINE_READER_H_

#include <APL/Uncopyable.h>
#include <APL/AsyncPhysLayerMonitor.h>
#include <APL/CopyableBuffer.h>
#include <string>

namespace apl {

class IPhysicalLayerAsync;
class ITimerSource;

/** Abstracts the process of reading line from a physical layer.
*/
class LineReader : public AsyncPhysLayerMonitor, private Uncopyable
{
	public:
		LineReader(Logger* apLogger, IPhysicalLayerAsync* apPhysical, ITimerSource* apTimerSrc, size_t aBuffSize);

		virtual void AcceptLine(const std::string&) = 0;
		virtual void _Up() = 0;
		virtual void _Down() = 0;

	private:
		CopyableBuffer mBuffer;
		size_t mNumBytes;
		bool mHasCR;

		void Read();
		void Up();
		void Down();
		void Reset();

		void _OnReceive(const apl::byte_t*, size_t aNum);
		void ReadBuffer();
};


}

#endif
