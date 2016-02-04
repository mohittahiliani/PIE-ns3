/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2016 NITK Surathkal
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Authors: Shravya Ks <shravya.ks0@gmail.com>,
 *          Smriti Murali <m.smriti.95@gmail.com>,
 *          Virang vyas <virangnitk@gmail.com>,
 *          Mohit P. Tahiliani <tahiliani@nitk.edu.in>
 */

/*
 * PORT NOTE: This code was ported from ns-2 (queue/pie.h).
 *
 * This file incorporates work covered by the following copyright and
 * permission notice:
 *
 * Proportional Integral Controller - Enhanced (PIE) AQM Implementation
 *
 * Authors of the code:
 * Preethi Natarajan (prenatar@cisco.com)
 * Rong Pan (ropan@cisco.com)
 * Chiara Piglione (cpiglion@cisco.com)
 * Greg White (g.white@cablelabs.com)
 * Takashi Hayakawa (t.hayakawa@cablelabs.com)

 * Copyright (c) 2013-2014, Cisco Systems, Inc.
 * Portions Copyright (c) 2013-2014 Cable Television Laboratories, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following
 * conditions are met:
 *   -	Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
 *   -	Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer
 *	in the documentation and/or other materials provided with the distribution.
 *   -	Neither the name of Cisco Systems, Inc. nor the names of its contributors may be used to endorse or promote products derived
 *	from this software without specific prior written permission.
 * ALL MATERIALS ARE PROVIDED BY CISCO AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, AND NONINFRINGEMENT ARE DISCLAIMED.  IN NO EVENT SHALL
 * CISCO OR ANY CONTRIBUTOR BE LIABLE FOR ANY DIRECT, INDIRECT, SPECIAL, EXEMPLARY, CONSEQUENTIAL, OR INCIDENTAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.
 *
 */

#ifndef PIE_QUEUE_H
#define PIE_QUEUE_H

#include <queue>
#include "ns3/packet.h"
#include "ns3/queue.h"
#include "ns3/nstime.h"
#include "ns3/boolean.h"
#include "ns3/data-rate.h"
#include "ns3/nstime.h"
#include "ns3/timer.h"
#include "ns3/event-id.h"

#define PROB_LOW 0.85
#define PROB_HIGH 8.5
#define MIN_PKTSIZE 64
#define BURST_RESET_TIMEOUT 1.5
#define LATENCY_LOW 0.005

namespace ns3 {

class TraceContainer;
class UniformRandomVariable;

/**
 * \ingroup queue
 *
 * \brief A PIE packet queue
 */
class PieQueue : public Queue
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);
  /**
   * \brief PieQueue Constructor
   *
   * Create a PIE queue
   */
  PieQueue ();

  /**
   * \brief Destructor
   *
   * Destructor
   */
  virtual ~PieQueue ();

  /**
   * \brief Stats
   */
  typedef struct
  {
    uint32_t unforcedDrop;                      //!< Unforced drop: proactive
    uint32_t forcedDrop;                        //!< Forced drop: reactive to full queue
  } Stats;

  /**
   * \brief Burst types
   */
  enum BurstStateT
  {
    NO_BURST,
    IN_BURST,
    IN_BURST_PROTECTING,
  };

  /**
   * \brief Get the PIE statistics after running.
   *
   * \returns The drop statistics.
   */
  Stats GetStats ();

  /**
   * \brief Get the current value of the queue
   *
   * \returns The queue size
   */
  uint32_t GetQueueSize (void);

  /**
   * \brief Get the drop count
   */
  uint32_t GetDropCount (void);

  /**
   * \brief Get queue delay
   */
  double GetQueueDelay (void);

  /**
   * \brief Get the drop probability
   */
  double GetDropProb (void);

  /**
   * \brief Set the limit of the queue.
   *
   * \param lim The limit of the queue.
   */
  void SetQueueLimit (uint32_t);


  /**
 * Assign a fixed random variable stream number to the random variables
 * used by this model.  Return the number of streams (possibly zero) that
 * have been assigned.
 *
 * \param stream first stream index to use
 * \return the number of stream indices assigned by this model
 */
  int64_t AssignStreams (int64_t stream);


private:
  virtual bool DoEnqueue (Ptr<Packet> p);
  virtual Ptr<Packet> DoDequeue (void);
  virtual Ptr<const Packet> DoPeek (void) const;

  /**
  * \brief Initialize the queue parameters.
  */
  void InitializeParams (void);

  /**
   * Reinitialise every members for next run
   */
  void Reset ();

  /**
   * \brief Check if packet p needs to be dropped due to probability mark
   * \param pkt packet
   * \param qlen queue length
   * \param qlim queue limit
   * \returns 0 for no drop/mark, 1 for drop
   */
  bool DropEarly (Ptr<Packet> pkt, uint32_t qlen);

  /**
   * Periodically update the drop probability based on the delay samples:
   * not only the current delay sample but also the trend where the delay
   * is going, up or down
   */
  void CalculateP ();

  std::queue<Ptr<Packet> > m_packets;           //!< packets in the queue
  uint32_t m_bytesInQueue;                      //!< bytes in the queue
  bool m_hasPieStarted;                         //!< True if PIE has started
  Stats m_stats;                                //!< PIE statistics


  // ** Variables supplied by user
  int m_qLim_;                                  //!< Maintains the maximum limit of the queue
  bool m_queueInBytes;                          //!< Checks if queue is in byte mode
  Time m_sUpdate;                               //!< Time to start the update timer
  Time m_tUpdate;                               //!< sampling timer
  Time m_qDelayRef;                             //!< desired queue delay
  uint32_t m_meanPktSize;                       //!< avg pkt size, linked into Tcl
  double m_maxBurst;                            //!< maximum burst allowed before random early dropping kicks in
  Time m_linkDelay;                             //!< Link delay
  DataRate m_linkBandwidth;                     //!< Link bandwidth
  double m_a;                                   //!< parameters to pie controller
  double m_b;                                   //!< parameters to pie controller
  uint32_t m_dqThreshold;                       //!< threshold that needs to be across before a sample of the dequeue rate is measured


  // ** Variables maintained by PIE
  double m_dropProb;                            //!< Variable used in calculation of drop probability
  double m_accuProb;                            //!< Variable used in calculation of drop probability
  Time m_qDelayOld;                             //!< Measures the old value of queue delay
  Time m_qDelay;                                //!< Measures the value of queue delay
  double m_burstAllowance;                      //!< current max burst size that is allowed before random drops kick in
  uint32_t m_burstReset;                        //!< Used to reset value of burst allowance
  BurstStateT m_burstState;                     //!< Used to determine the current state of burst
  uint32_t m_inMeasurement;                     //!< indicate whether we are in a measurement cycle
  double m_avgDqRate;                           //!< time averaged dequeue rate
  double m_dqStart;                             //!< the start timestamp of current measurement cycle
  uint32_t m_dqCount;                           //!< number of bytes departed since current measurement cycle starts
  uint32_t m_curq;                              //!< helps to trace queue during arrival, if enabled
  EventId m_rtrsEvent;                          //!< Event used to decide the decision of interval of drop probability calculation
  Ptr<UniformRandomVariable> m_uv;              //!< rng stream

};

};   // namespace ns3

#endif

