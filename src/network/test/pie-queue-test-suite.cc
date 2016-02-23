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
 *
 */

#include "ns3/test.h"
#include "ns3/pie-queue.h"
#include "ns3/uinteger.h"
#include "ns3/string.h"
#include "ns3/double.h"
#include "ns3/log.h"
#include "ns3/simulator.h"

using namespace ns3;

class PieQueueTestCase : public TestCase
{
public:
  PieQueueTestCase ();
  virtual void DoRun (void);
private:
  void Enqueue (Ptr<PieQueue> queue, uint32_t size, uint32_t nPkt);
  void RunPieTest ();
};

PieQueueTestCase::PieQueueTestCase ()
  : TestCase ("Sanity check on the pie queue implementation")
{
}

void
PieQueueTestCase::RunPieTest ()
{
  uint32_t pktSize = 0;
  uint32_t modeSize = 1;
  // 1 for packets; pktSize for bytes
  uint32_t qSize = 8;
  Ptr<PieQueue> queue = CreateObject<PieQueue> ();

  // test 1: simple enqueue/dequeue with no drops
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("QueueLimit", UintegerValue (25)),
                         true,"Verify that we can actually set the attribute QueueLimit");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("QueueDelayReference", TimeValue (Seconds (0.002))), true,
                         "Verify that we can actually set the attribute QW");
  NS_LOG_UNCOND ("test 1  verified");


  modeSize = 1000;
  pktSize = 1000;
  queue->SetQueueLimit (qSize);

  Ptr<Packet> p1, p2, p3, p4, p5, p6, p7, p8;
  p1 = Create<Packet> (pktSize);
  p2 = Create<Packet> (modeSize);
  p3 = Create<Packet> (pktSize);
  p4 = Create<Packet> (pktSize);
  p5 = Create<Packet> (pktSize);
  p6 = Create<Packet> (pktSize);
  p7 = Create<Packet> (pktSize);
  p8 = Create<Packet> (pktSize);

  NS_TEST_EXPECT_MSG_EQ (queue->GetQueueSize (), 0 * modeSize, "There should be no packets in there");
  queue->Enqueue (p1);
  NS_TEST_EXPECT_MSG_EQ (queue->GetQueueSize (), 1 * modeSize, "There should be one packet in there");
  queue->Enqueue (p2);
  NS_TEST_EXPECT_MSG_EQ (queue->GetQueueSize (), 2 * modeSize, "There should be two packets in there");
  queue->Enqueue (p3);
  queue->Enqueue (p4);
  queue->Enqueue (p5);
  queue->Enqueue (p6);
  queue->Enqueue (p7);
  queue->Enqueue (p8);
  NS_TEST_EXPECT_MSG_EQ (queue->GetQueueSize (), 8 * modeSize, "There should be eight packets in there");

  Ptr<Packet> p;

  p = queue->Dequeue ();
  NS_TEST_EXPECT_MSG_EQ ((p != 0), true, "I want to remove the first packet");
  NS_TEST_EXPECT_MSG_EQ (queue->GetQueueSize (), 7 * modeSize, "There should be seven packets in there");
  NS_TEST_EXPECT_MSG_EQ (p->GetUid (), p1->GetUid (), "Was this the first packet ?");

  p = queue->Dequeue ();
  NS_TEST_EXPECT_MSG_EQ ((p != 0), true, "I want to remove the second packet");
  NS_TEST_EXPECT_MSG_EQ (queue->GetQueueSize (), 6 * modeSize, "There should be six packet in there");
  NS_TEST_EXPECT_MSG_EQ (p->GetUid (), p2->GetUid (), "Was this the second packet ?");

  p = queue->Dequeue ();
  NS_TEST_EXPECT_MSG_EQ ((p != 0), true, "I want to remove the third packet");
  NS_TEST_EXPECT_MSG_EQ (queue->GetQueueSize (), 5 * modeSize, "There should be five packets in there");
  NS_TEST_EXPECT_MSG_EQ (p->GetUid (), p3->GetUid (), "Was this the third packet ?");

  p = queue->Dequeue ();
  p = queue->Dequeue ();
  p = queue->Dequeue ();
  p = queue->Dequeue ();
  p = queue->Dequeue ();
  p = queue->Dequeue ();
  NS_TEST_EXPECT_MSG_EQ ((p == 0), true, "There are really no packets in there");

  // test 2: more data, but with no drops
  queue = CreateObject<PieQueue> ();
  qSize = 300 * modeSize;
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("QueueLimit", UintegerValue (qSize)), true,
                         "Verify that we can actually set the attribute QueueLimit");
  Enqueue (queue, pktSize, 300);
  PieQueue::Stats st = StaticCast<PieQueue> (queue)->GetStats ();
  NS_TEST_EXPECT_MSG_EQ (st.unforcedDrop, 0, "There should be zero dropped packets due hardmark mark");
  NS_TEST_EXPECT_MSG_EQ (st.forcedDrop, 0, "There should be zero dropped packets due queue full");

  // test 3: more data, but with drops
  queue = CreateObject<PieQueue> ();
  qSize = modeSize;
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("MaxBurstAllowance",DoubleValue (0.1)), true,
                         "Verify that we can actually set the attribute A");
  Enqueue (queue, pktSize, 300000);
  PieQueue::Stats st2 = StaticCast<PieQueue> (queue)->GetStats ();
  int drop1;
  drop1 = st2.unforcedDrop + st2.forcedDrop;
  NS_TEST_EXPECT_MSG_NE (drop1, 0, "There should be some dropped packets");

  // test 4: more burst allowance implies more packet drops
  queue = CreateObject<PieQueue> ();
  qSize = 150 * modeSize;
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("MaxBurstAllowance",DoubleValue (0.4)), true,
                         "Verify that we can actually set the attribute A");
  Enqueue (queue, pktSize, 3000000);
  PieQueue::Stats st1 = StaticCast<PieQueue> (queue)->GetStats ();
  int drop4;
  drop4 = st1.unforcedDrop + st1.forcedDrop;
  NS_TEST_EXPECT_MSG_NE (drop4, 0, "There should be some dropped packets");

  //test 5: more burst allowance implies more packet drops
  queue = CreateObject<PieQueue> ();
  qSize = 150 * modeSize;
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("QueueLimit", UintegerValue (qSize)), true,
                         "Verify that we can actually set the   attribute QueueLimit");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("MaxBurstAllowance",DoubleValue (0.1)), true,
                         "Verify that we can actually set the attribute A");
  Enqueue (queue, pktSize, 3000000);
  PieQueue::Stats stat = StaticCast<PieQueue> (queue)->GetStats ();
  int drop5 = stat.unforcedDrop + stat.forcedDrop;
  NS_TEST_EXPECT_MSG_NE (drop5, 0, "There should be some dropped packets");
  NS_TEST_EXPECT_MSG_GT (drop4, drop5, "Test 4 should have more drops than test 5");
}

void PieQueueTestCase::Enqueue (Ptr<PieQueue> queue, uint32_t size, uint32_t nPkt)
{
  for (uint32_t i = 0; i < nPkt; i++)
    {
      queue->Enqueue (Create<Packet> (size));
    }
}

void
PieQueueTestCase::DoRun (void)
{
  RunPieTest ();
  Simulator::Destroy ();
}

static class PieQueueTestSuite : public TestSuite
{
public:
  PieQueueTestSuite ()
    : TestSuite ("pie-queue", UNIT)
  {
    AddTestCase (new PieQueueTestCase (), TestCase::QUICK);
  }
} g_pieQueueTestSuite;
