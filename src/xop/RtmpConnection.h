#ifndef XOP_RTMP_CONNECTION_H
#define XOP_RTMP_CONNECTION_H

#include "net/EventLoop.h"
#include "net/TcpConnection.h"
#include "amf.h"
#include "rtmp.h"
#include <vector>

namespace xop
{

class RtmpServer;

// chunk header: basic_header + rtmp_message_header 
struct RtmpMessageHeader
{
    uint8_t timestamp[3];
    uint8_t length[3];
    uint8_t typeId;
    uint8_t streamId[4]; //小端格式
};

struct RtmpMessage 
{
    uint32_t timestamp = 0;
    uint32_t length = 0;
    uint8_t  typeId = 0;
    uint32_t streamId = 0;
    uint32_t extTimestamp = 0;   
	
	uint64_t _timestamp = 0;
	uint8_t  codecId = 0;

    uint8_t  csid = 0; 
    uint32_t index = 0;
    std::shared_ptr<char> payload = nullptr;

    void reset()
    {        
        index = 0;    
		timestamp = 0;
		extTimestamp = 0;
    }
};

class RtmpConnection : public TcpConnection
{
public:    
    enum ConnectionState
    {
        HANDSHAKE_C0C1, 
        HANDSHAKE_C2,
        HANDSHAKE_COMPLETE,
        START_PLAY,
        START_PUBLISH
    };
      
	enum ChunkParseState
	{
		PARSE_HEADER, 
		PARSE_BODY, 
	};
  
    RtmpConnection() = delete;
    RtmpConnection(RtmpServer* rtmpServer, TaskScheduler* taskScheduler, SOCKET sockfd);
    ~RtmpConnection();

    std::string getStreamPath() const
    { return m_streamPath; }

    std::string getStreamName() const
    { return m_streamName; }

    std::string getApp() const
    { return m_app; }

    AmfObjects getMetaData() const 
    { return m_metaData; }

    bool isPlayer() const 
    { return m_connState == START_PLAY; }

    bool isPublisher() const 
    { return m_connState == START_PUBLISH; }
    
private:
    friend class RtmpSession;

    bool onRead(BufferReader& buffer);
    void onClose();

	int parseChunkHeader(BufferReader& buffer);
	int parseChunkBody(BufferReader& buffer);

    bool handleHandshake(BufferReader& buffer);
    bool handleChunk(BufferReader& buffer);
    bool handleMessage(RtmpMessage& rtmpMsg);
    bool handleInvoke(RtmpMessage& rtmpMsg);
    bool handleNotify(RtmpMessage& rtmpMsg);
    bool handleVideo(RtmpMessage& rtmpMsg);
    bool handleAudio(RtmpMessage& rtmpMsg);

    bool handleConnect();
    bool handleCreateStream();
    bool handlePublish();
    bool handlePlay();
    bool handlePlay2();
    bool handDeleteStream();

    void setPeerBandwidth();
    void sendAcknowledgement();
    void setChunkSize();

    bool sendInvokeMessage(uint32_t csid, std::shared_ptr<char> payload, uint32_t payloadSize);
    bool sendNotifyMessage(uint32_t csid, std::shared_ptr<char> payload, uint32_t payloadSize);   
    bool sendMetaData(AmfObjects& metaData);
    bool sendMediaData(uint8_t type, uint64_t timestamp, std::shared_ptr<char> payload, uint32_t payloadSize);
	bool sendVideoData(uint64_t timestamp, std::shared_ptr<char> payload, uint32_t payloadSize);
	bool sendAudioData(uint64_t timestamp, std::shared_ptr<char> payload, uint32_t payloadSize);
    void sendRtmpChunks(uint32_t csid, RtmpMessage& rtmpMsg);
    int createChunkBasicHeader(uint8_t fmt, uint32_t csid, char* buf);
    int createChunkMessageHeader(uint8_t fmt, RtmpMessage& rtmpMsg, char* buf);   

	RtmpServer *m_rtmpServer;
	TaskScheduler *m_taskScheduler;
	std::shared_ptr<xop::Channel> m_channelPtr;

	std::string m_app;
	std::string m_streamName;
	std::string m_streamPath;
	AmfObjects m_metaData;
	AmfDecoder m_amfDec;
	AmfEncoder m_amfEnc;
	std::map<int, RtmpMessage> m_rtmpMessasges;
	ConnectionState m_connState = HANDSHAKE_C0C1;
	ChunkParseState m_chunkParseState = PARSE_HEADER;
	int m_chunkStreamId = 0;
	uint32_t m_inChunkSize = 128;
	uint32_t m_outChunkSize = 128;
	uint32_t m_streamId = 0;
	bool hasKeyFrame = false;
	bool m_enableGopCache = true;
	std::map<uint64_t, RtmpMessage> m_gopCache;
	std::shared_ptr<char> m_avcSequenceHeader;
	std::shared_ptr<char> m_aacSequenceHeader;
	uint32_t m_avcSequenceHeaderSize = 0;
	uint32_t m_aacSequenceHeaderSize = 0;
    
	const uint32_t kPeerBandwidth       = 5000000;
	const uint32_t kAcknowledgementSize = 5000000;
	const uint32_t kMaxChunkSize        = 60000;
	const uint32_t kStreamId            = 1;
	const uint32_t kMaxGopLen           = 2000;
	const int kChunkMessageLen[4] = { 11, 7, 3, 0 };
};
      
}

#endif
