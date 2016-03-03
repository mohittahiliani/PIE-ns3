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
 *          Mohit P. Tahiliani <tahiliani@nitk.edu.in>
 */

/*
 * PORT NOTE: This code was ported from ns-2 (queue/pie.cc).  Almost all
 * comments have also been ported from ns-2
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

#include "ns3/log.h"
#include "ns3/enum.h"
#include "ns3/uinteger.h"
#include "ns3/double.h"
#include "ns3/simulator.h"
#include "ns3/abort.h"
#include "ns3/random-variable-stream.h"
#include "pie-queue.h"
#include "ns3/timer.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("PieQueue");

NS_OBJECT_ENSURE_REGISTERED (PieQueue);

TypeId PieQueue::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::PieQueue")
    .SetParent<Queue> ()
    .SetGroupName ("Network")
    .AddConstructor<PieQueue> ()
    .AddAttribute ("Mode",
                   "Determines unit for QueueLimit",
                   EnumValue (QUEUE_MODE_PACKETS),
                   MakeEnumAccessor (&PieQueue::SetMode),
                   MakeEnumChecker (QUEUE_MODE_BYTES, "QUEUE_MODE_BYTES",
                                    QUEUE_MODE_PACKETS, "QUEUE_MODE_PACKETS"))
    .AddAttribute ("MeanPktSize",
                   "Average of packet size",
                   UintegerValue (1000),
                   MakeUintegerAccessor (&PieQueue::m_meanPktSize),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("LinkBandwidth",
                   "The PIE link bandwidth",
                   DataRateValue (DataRate ("1.5Mbps")),
                   MakeDataRateAccessor (&PieQueue::m_linkBandwidth),
                   MakeDataRateChecker ())
    .AddAttribute ("LinkDelay",
                   "The PIE link delay",
                   TimeValue (MilliSeconds (20)),
                   MakeTimeAccessor (&PieQueue::m_linkDelay),
                   MakeTimeChecker ())
    .AddAttribute ("A",
                   "Value of a",
                   DoubleValue (0.125),
                   MakeDoubleAccessor (&PieQueue::m_a),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("B",
                   "Value of b",
                   DoubleValue (1.25),
                   MakeDoubleAccessor (&PieQueue::m_b),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("Tupdate",
                   "Time period to calculate drop probability",
                   TimeValue (Seconds (0.03)),
                   MakeTimeAccessor (&PieQueue::m_tUpdate),
                   MakeTimeChecker ())
    .AddAttribute ("Supdate",
                   "Start time of the update timer",
                   TimeValue (Seconds (0)),
                   MakeTimeAccessor (&PieQueue::m_sUpdate),
                   MakeTimeChecker ())
    .AddAttribute ("QueueLimit",
                   "Queue limit in bytes/packets",
                   UintegerValue (25),
                   MakeUintegerAccessor (&PieQueue::m_qLim),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("DequeueThreshold",
                   "Minimum queue size in bytes before dequeue rate is measured",
                   UintegerValue (10000),
                   MakeUintegerAccessor (&PieQueue::m_dqThreshold),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("QueueDelayReference",
                   "Desired queue delay",
                   TimeValue (Seconds (0.02)),
                   MakeTimeAccessor (&PieQueue::m_qDelayRef),
                   MakeTimeChecker ())
    .AddAttribute ("MaxBurstAllowance",
                   "Current max burst allowance in seconds before random drop",
                   TimeValue (Seconds (0.1)),
                   MakeTimeAccessor (&PieQueue::m_maxBurst),
                   MakeTimeChecker ())
  ;

  return tid;
}

PieQueue::PieQueue ()
  : Queue (),
    m_packets (),
    m_bytesInQueue (0),
    m_hasPieStarted (false)
{
  NS_LOG_FUNCTION (this);

  InitializeParams ();
  m_uv = CreateObject<UniformRandomVariable> ();
  m_rtrsEvent = Simulator::Schedule (m_sUpdate, &PieQueue::CalculateP, this);
}

PieQueue::~PieQueue ()
{
  NS_LOG_FUNCTION (this);
}

void
PieQueue::SetMode (PieQueue::QueueMode mode)
{
  NS_LOG_FUNCTION (this << mode);
  m_mode = mode;
}

PieQueue::QueueMode
PieQueue::GetMode (void)
{
  NS_LOG_FUNCTION (this);
  return m_mode;
}

void
PieQueue::SetQueueLimit (uint32_t lim)
{
  NS_LOG_FUNCTION (this << lim);
  m_qLim = lim;
}

uint32_t
PieQueue::GetQueueSize (void)
{
  NS_LOG_FUNCTION (this);
  if (GetMode () == QUEUE_MODE_BYTES)
    {
      return m_bytesInQueue;
    }
  else if (GetMode () == QUEUE_MODE_PACKETS)
    {
      return m_packets.size ();
    }
  else
    {
      NS_ABORT_MSG ("Unknown PIE mode.");
    }
}

PieQueue::Stats
PieQueue::GetStats ()
{
  NS_LOG_FUNCTION (this);
  return m_stats;
}

Time
PieQueue::GetQueueDelay (void)
{
  NS_LOG_FUNCTION (this);
  return m_qDelay;
}

int64_t
PieQueue::AssignStreams (int64_t stream)
{
  NS_LOG_FUNCTION (this << stream);
  m_uv->SetStream (stream);
  return 1;
}

bool
PieQueue::DoEnqueue (Ptr<Packet> pkt)
{
  NS_LOG_FUNCTION (this << pkt);
  if (!m_hasPieStarted )
    {
      m_hasPieStarted = true;
    }

  uint32_t QLen = 0;

  if (GetMode () == QUEUE_MODE_BYTES)
    {
      QLen = m_bytesInQueue;
    }
  else if (GetMode () == QUEUE_MODE_PACKETS)
    {
      QLen = m_packets.size ();
    }
  if (QLen >= m_qLim)
    {
      // Drops due to queue limits: reactive
      Drop (pkt);
      m_stats.forcedDrop++;
    }
  else if (DropEarly (pkt, QLen))
    {
      // Early probability drop: proactive
      Drop (pkt);
      m_stats.unforcedDrop++;
    }
  else
    {
      // No drop
      m_packets.push (pkt);
      m_bytesInQueue += pkt->GetSize ();
    }

  NS_LOG_LOGIC ("\t bytesInQueue  " << m_bytesInQueue );
  NS_LOG_LOGIC ("\t packetsInQueue  " << m_packets.size () );
  return true;
}

void
PieQueue::InitializeParams (void)
{
  // Initially queue is empty so variables are initialize to zero except m_dqCount
  m_inMeasurement = 0;
  m_dqCount = -1;
  m_dropProb = 0;
  m_avgDqRate = 0.0;
  m_dqStart = 0;
  m_bytesInQueue = 0;
  m_burstState = NO_BURST;
  m_qDelayOld = Time (Seconds (0));
  m_stats.forcedDrop = 0;
  m_stats.unforcedDrop = 0;
}

void
PieQueue::Reset ()
{
  NS_LOG_FUNCTION (this);
  Ptr<Packet> p;
  InitializeParams ();
  Simulator::Remove (m_rtrsEvent);
  m_rtrsEvent = Simulator::Schedule (m_sUpdate, &PieQueue::CalculateP, this);
  while ((p = m_packets.front ()) != 0)
    {
      m_packets.pop ();
    }
  m_bytesInQueue = 0;
}

bool PieQueue::DropEarly (Ptr<Packet> pkt, uint32_t QLen)
{
  NS_LOG_FUNCTION (this << pkt << QLen);
  if (m_burstAllowance.GetSeconds () > 0)
    {
      // If there is still burst_allowance left, skip random early drop.
      return false;
    }

  if (m_burstState == NO_BURST)
    {
      m_burstState = IN_BURST_PROTECTING;
      m_burstAllowance = m_maxBurst;
    }

  double p = m_dropProb;

  uint32_t packetSize = pkt->GetSize ();

  if (GetMode () == QUEUE_MODE_BYTES)
    {
      p = p * packetSize / m_meanPktSize;
    }
  bool earlyDrop = true;
  double u =  m_uv->GetValue ();

  if (m_qDelayOld.GetSeconds () < 0.5 * m_qDelayRef.GetSeconds () && m_dropProb < 0.2)
    {
      return false;
    }
  else if (GetMode () == QUEUE_MODE_BYTES && QLen <= 2 * m_meanPktSize)
    {
      return false;
    }
  else if (GetMode () == QUEUE_MODE_PACKETS && QLen <= 2)
    {
      return false;
    }

  if (u > p)
    {
      earlyDrop = false;
    }
  if (earlyDrop == false)
    {
      return false;
    }

  return true;
}

void PieQueue::CalculateP ()
{
  NS_LOG_FUNCTION (this);
  Time qDelay;
  double p;
  bool missingInitFlag = false;
  if (m_avgDqRate > 0)
    {
      qDelay = Time (Seconds (m_bytesInQueue / m_avgDqRate));
    }
  else
    {
      qDelay = Time (Seconds (0));
      missingInitFlag = true;
    }

  m_qDelay = qDelay;

  if (m_burstAllowance.GetSeconds () > 0)
    {
      m_dropProb = 0;
    }
  else
    {
      p = m_a * (qDelay.GetSeconds () - m_qDelayRef.GetSeconds ()) + m_b * (qDelay.GetSeconds () - m_qDelayOld.GetSeconds ());
      if (m_dropProb < 0.001)
        {
          p /= 32;
        }
      else if (m_dropProb < 0.01)
        {
          p /= 8;
        }
      else if (m_dropProb < 0.1)
        {
          p /= 2;
        }
      else if (m_dropProb < 1)
        {
          p /= 0.5;
        }
      else if (m_dropProb < 10)
        {
          p /= 0.125;
        }
      else
        {
          p /= 0.03125;
        }
      if ((m_dropProb >= 0.1) && (p > 0.02))
        {
          p = 0.02;
        }
    }

  p += m_dropProb;

  // For non-linear drop in prob

  if (qDelay.GetSeconds () == 0 && m_qDelayOld.GetSeconds () == 0)
    {
      p *= 0.98;
    }
  else if (qDelay.GetSeconds () > 0.2)
    {
      p += 0.02;
    }

  m_dropProb = (p > 0) ? p : 0;
  if (m_burstAllowance < m_tUpdate)
    {
      m_burstAllowance =  Time (Seconds (0));
    }
  else
    {
      m_burstAllowance -= m_tUpdate;
    }

  uint32_t burstResetLimit = BURST_RESET_TIMEOUT / m_tUpdate.GetSeconds ();
  if ( (qDelay.GetSeconds () < 0.5 * m_qDelayRef.GetSeconds ()) && (m_qDelayOld.GetSeconds () < (0.5 * m_qDelayRef.GetSeconds ())) && (m_dropProb == 0) && !missingInitFlag )
    {
      m_dqCount = -1;
      m_avgDqRate = 0.0;
    }
  if (qDelay.GetSeconds () < 0.5 * m_qDelayRef.GetSeconds () && m_qDelayOld.GetSeconds () < 0.5 * m_qDelayRef.GetSeconds () && m_dropProb == 0 && m_burstAllowance.GetSeconds () == 0)
    {
      if (m_burstState == IN_BURST_PROTECTING)
        {
          m_burstState = IN_BURST;
          m_burstReset = 0;
        }
      else if (m_burstState == IN_BURST)
        {
          m_burstReset++;
          if (m_burstReset > burstResetLimit)
            {
              m_burstReset = 0;
              m_burstState = NO_BURST;
            }
        }
    }
  else if (m_burstState == IN_BURST)
    {
      m_burstReset = 0;
    }

  m_qDelayOld = qDelay;
  Simulator::Remove (m_rtrsEvent);
  m_rtrsEvent = Simulator::Schedule (m_tUpdate, &PieQueue::CalculateP, this);
}

Ptr<Packet> PieQueue::DoDequeue ()
{
  NS_LOG_FUNCTION (this);
  Ptr<Packet> p;
  if (m_packets.empty ())
    {
      return 0;
    }
  else
    {
      p = m_packets.front ();
      m_packets.pop ();
      m_bytesInQueue -= p->GetSize ();
      double now = Simulator::Now ().GetSeconds ();
      uint32_t pktSize = p->GetSize ();

      /* if not in a measurement cycle and the queue has built up to dq_threshold,
      start the measurement cycle*/

      if ( (m_bytesInQueue >= (uint32_t )(m_dqThreshold)) && (m_inMeasurement == 0) )
        {
          m_dqStart = now;
          m_dqCount = 0;
          m_inMeasurement = 1;
        }

      if (m_inMeasurement == 1)
        {
          m_dqCount += pktSize;

          // done with a measurement cycle
          if (m_dqCount >= (m_dqThreshold))
            {

              double tmp = now - m_dqStart;

              if (m_avgDqRate == 0)
                {
                  m_avgDqRate = m_dqCount / tmp;
                }
              else
                {
                  m_avgDqRate = 0.5 * m_avgDqRate + 0.5 * m_dqCount / tmp;
                }

              // restart a measurement cycle if there is enough data
              if (m_bytesInQueue > (uint32_t) (m_dqThreshold))
                {
                  m_dqStart = now;
                  m_dqCount = 0;
                  m_inMeasurement = 1;
                }
              else
                {
                  m_dqCount = 0;
                  m_inMeasurement = 0;
                }
            }
        }

      return (p);
    }
}

Ptr<const Packet>
PieQueue::DoPeek () const
{
  NS_LOG_FUNCTION (this);
  if (m_packets.empty ())
    {
      return 0;
    }
  Ptr<Packet> p = m_packets.front ();
  return p;
}
} //namespace ns3
