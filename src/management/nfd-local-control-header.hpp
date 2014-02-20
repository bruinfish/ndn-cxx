/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/**
 * Copyright (C) 2013 Regents of the University of California.
 * See COPYING for copyright and distribution information.
 */

#ifndef NDN_MANAGEMENT_NFD_LOCAL_CONTROL_HEADER_HPP
#define NDN_MANAGEMENT_NFD_LOCAL_CONTROL_HEADER_HPP

#include "../encoding/encoding-buffer.hpp"
#include "../encoding/tlv-nfd.hpp"

namespace ndn {
namespace nfd {

class LocalControlHeader
{
public:
  struct Error : public std::runtime_error { Error(const std::string &what) : std::runtime_error(what) {} };

  LocalControlHeader()
    : m_incomingFaceId(INVALID_FACE_ID)
    , m_nextHopFaceId(INVALID_FACE_ID)
  {
  }

  /**
   * @brief Create wire encoding with options LocalControlHeader and the supplied item
   *
   * The caller is responsible of checking whether LocalControlHeader contains
   * any information.
   *
   * !It is an error to call this method if neither IncomingFaceId nor NextHopFaceId is
   * set, or neither of them is enabled.
   *
   * @throws LocalControlHeader::Error when empty LocalControlHeader be produced
   *
   * @returns Block, containing LocalControlHeader. Top-level length field of the
   *          returned LocalControlHeader includes payload length, but the memory
   *          block is independent of the payload's wire buffer.  It is expected
   *          that both LocalControlHeader's and payload's wire will be send out
   *          together within a single send call.
   *
   * @see http://redmine.named-data.net/projects/nfd/wiki/LocalControlHeader
   */
  template<class U>
  inline Block
  wireEncode(const U& payload,
             bool encodeIncomingFaceId, bool encodeNextHopFaceId) const;
  
  /**
   * @brief Decode from the wire format and set LocalControlHeader on the supplied item
   *
   * The supplied wire MUST contain LocalControlHeader.  Determination whether the optional
   * LocalControlHeader should be done before calling this method.
   */
  inline void 
  wireDecode(const Block& wire,
             bool encodeIncomingFaceId = true, bool encodeNextHopFaceId = true);

  inline static const Block&
  getPayload(const Block& wire);
  
  ///////////////////////////////////////////////////////////////////////////////
  ///////////////////////////////////////////////////////////////////////////////
  ///////////////////////////////////////////////////////////////////////////////
  // Getters/setters

  bool
  empty(bool encodeIncomingFaceId, bool encodeNextHopFaceId) const
  {
    return !((encodeIncomingFaceId && hasIncomingFaceId()) ||
             (encodeNextHopFaceId  && hasNextHopFaceId()));
  }
  
  //
  
  bool
  hasIncomingFaceId() const
  {
    return m_incomingFaceId != INVALID_FACE_ID;
  }
  
  uint64_t
  getIncomingFaceId() const
  {
    return m_incomingFaceId;
  }

  void
  setIncomingFaceId(uint64_t incomingFaceId)
  {
    m_incomingFaceId = incomingFaceId;
  }

  //

  bool
  hasNextHopFaceId() const
  {
    return m_nextHopFaceId != INVALID_FACE_ID;
  }
  
  uint64_t
  getNextHopFaceId() const
  {
    return m_nextHopFaceId;
  }

  void
  setNextHopFaceId(uint64_t nextHopFaceId)
  {
    m_nextHopFaceId = nextHopFaceId;
  }

private:
  template<bool T>
  inline size_t
  wireEncode(EncodingImpl<T>& block, size_t payloadSize,
             bool encodeIncomingFaceId, bool encodeNextHopFaceId) const;
  
private:
  uint64_t m_incomingFaceId;
  uint64_t m_nextHopFaceId;
};


/**
 * @brief Fast encoding or block size estimation
 */
template<bool T>
inline size_t
LocalControlHeader::wireEncode(EncodingImpl<T>& block, size_t payloadSize,
                               bool encodeIncomingFaceId, bool encodeNextHopFaceId) const
{
  size_t total_len = payloadSize;

  if (encodeIncomingFaceId && hasIncomingFaceId())
    {
      total_len += prependNonNegativeIntegerBlock(block,
                                                  tlv::nfd::IncomingFaceId, getIncomingFaceId());
    }

  if (encodeNextHopFaceId && hasNextHopFaceId())
    {
      total_len += prependNonNegativeIntegerBlock(block,
                                                  tlv::nfd::NextHopFaceId, getNextHopFaceId());
    }
  
  total_len += block.prependVarNumber(total_len);
  total_len += block.prependVarNumber(tlv::nfd::LocalControlHeader);
  return total_len;
}

template<class U>
inline Block
LocalControlHeader::wireEncode(const U& payload,
                               bool encodeIncomingFaceId, bool encodeNextHopFaceId) const
{
  /// @todo should this be BOOST_ASSERT instead?  This is kind of unnecessary overhead
  if (empty(encodeIncomingFaceId, encodeNextHopFaceId))
    throw Error("Requested wire for LocalControlHeader, but none of the fields are set or enabled");

  EncodingEstimator estimator;
  size_t length = wireEncode(estimator, payload.wireEncode().size(),
                             encodeIncomingFaceId, encodeNextHopFaceId);
  
  EncodingBuffer buffer(length);
  wireEncode(buffer, payload.wireEncode().size(),
             encodeIncomingFaceId, encodeNextHopFaceId);

  return buffer.block(false);
}

inline void 
LocalControlHeader::wireDecode(const Block& wire,
                               bool encodeIncomingFaceId/* = true*/, bool encodeNextHopFaceId/* = true*/)
{
  BOOST_ASSERT(wire.type() == tlv::nfd::LocalControlHeader);
  wire.parse();

  m_incomingFaceId = INVALID_FACE_ID;
  m_nextHopFaceId = INVALID_FACE_ID;

  for (Block::element_const_iterator i = wire.elements_begin();
       i != wire.elements_end();
       ++i)
    {
      switch(i->type())
        {
        case tlv::nfd::IncomingFaceId:
          if (encodeIncomingFaceId)
            m_incomingFaceId = readNonNegativeInteger(*i);
          break;
        case tlv::nfd::NextHopFaceId:
          if (encodeNextHopFaceId)
            m_nextHopFaceId = readNonNegativeInteger(*i);
          break;
        default:
          // ignore all unsupported
          break;
        }
    }
}

inline const Block&
LocalControlHeader::getPayload(const Block& wire)
{
  if (wire.type() == tlv::nfd::LocalControlHeader)
    {
      wire.parse();
      if (wire.elements_size() < 1)
        return wire; // don't throw an error, but don't continue processing

      return wire.elements()[wire.elements().size()-1];
    }
  else
    {
      return wire;
    }
}


} // namespace nfd
} // namespace ndn

#endif // NDN_MANAGEMENT_NFD_LOCAL_CONTROL_HEADER_HPP
