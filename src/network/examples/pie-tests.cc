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

/** Network topology used for the example
 *
 *     10Mb/s,5ms
 * n8--------------|
 *                 |
 *    10Mb/s, 2ms  |                         10Mb/s, 4ms
 * n0--------------|                    |---------------n4
 *                 |   10Mbps/s, 50ms   |
 *                 n2------------------n3
 *    10Mb/s, 3ms  |                    |    10Mb/s, 5ms
 * n1--------------|                    |---------------n5
 *                 |
 *     10mb/s,3ms  |
 * n6--------------|
 *                 |
 *     10Mb/s,3ms  |
 * n7--------------|
 */

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/flow-monitor-helper.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include <fstream>
#include "ns3/internet-module.h"
#include "ns3/ipv6-static-routing-helper.h"
#include "ns3/ipv6-routing-table-entry.h"
#include "ns3/internet-module.h"
#include "ns3/flow-monitor-helper.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("PieTests");

uint32_t checkTimes;
double avgQueueSize;


double global_start_time;
double global_stop_time;
double sink_start_time;
double sink_stop_time;
double client_start_time;
double client_stop_time;

NodeContainer n0n2;
NodeContainer n1n2;
NodeContainer n2n3;
NodeContainer n3n4;
NodeContainer n3n5;
NodeContainer n6n2;
NodeContainer n7n2;
NodeContainer n8n2;

Ipv4InterfaceContainer i0i2;
Ipv4InterfaceContainer i1i2;
Ipv4InterfaceContainer i2i3;
Ipv4InterfaceContainer i3i4;
Ipv4InterfaceContainer i3i5;
Ipv4InterfaceContainer i6i2;
Ipv4InterfaceContainer i7i2;
Ipv4InterfaceContainer i8i2;

std::stringstream filePlotQueue;
std::stringstream filePlotQueueAvg;
std::stringstream filePlotQueueDel;
std::stringstream filePlotQueueNoDrop;
std::stringstream filePlotQueueDropProb;

void
CheckQueueSize (Ptr<Queue> queue)
{
  uint32_t qSize = StaticCast<PieQueue> (queue)->GetQueueSize ();

  avgQueueSize += qSize;

  checkTimes++;

  // check queue size every 1/100 of a second
  Simulator::Schedule (Seconds (0.01), &CheckQueueSize, queue);

  std::ofstream fPlotQueue (filePlotQueue.str ().c_str (), std::ios::out | std::ios::app);
  fPlotQueue << Simulator::Now ().GetSeconds () << " " << qSize << std::endl;
  fPlotQueue.close ();

  std::ofstream fPlotQueueAvg (filePlotQueueAvg.str ().c_str (), std::ios::out | std::ios::app);
  fPlotQueueAvg << Simulator::Now ().GetSeconds () << " " << avgQueueSize / checkTimes << std::endl;
  fPlotQueueAvg.close ();
}

void
CheckQueueDel (Ptr<Queue> queue)
{
  double qDel = StaticCast<PieQueue> (queue)->GetQueueDelay ();

  // check queue size every 1/100 of a second
  Simulator::Schedule (Seconds (0.01), &CheckQueueDel, queue);

  std::ofstream fPlotQueueDel (filePlotQueueDel.str ().c_str (), std::ios::out | std::ios::app);
  fPlotQueueDel << Simulator::Now ().GetSeconds () << " " << (qDel * 1000) << std::endl;
  fPlotQueueDel.close ();
}

void
CheckQueueProb (Ptr<Queue> queue)
{
  uint32_t ndrop = StaticCast<PieQueue> (queue)->GetDropCount ();
  double prob = StaticCast<PieQueue> (queue)->GetDropProb ();

  // check queue size every 1/100 of a second
  Simulator::Schedule (Seconds (0.01), &CheckQueueProb, queue);

  std::ofstream fPlotQueueNoDrop (filePlotQueueNoDrop.str ().c_str (), std::ios::out | std::ios::app);
  fPlotQueueNoDrop << Simulator::Now ().GetSeconds () << " " << ndrop << std::endl;
  fPlotQueueNoDrop.close ();

  std::ofstream fPlotQueueDropProb (filePlotQueueDropProb.str ().c_str (), std::ios::out | std::ios::app);
  fPlotQueueDropProb << Simulator::Now ().GetSeconds () << " " << prob << std::endl;
  fPlotQueueDropProb.close ();
}

void
BuildAppsTest (uint32_t test)
{

  if ( test == 1 )
    {
      // SINK is in the right side
      uint16_t port = 50000;
      Address sinkLocalAddress (InetSocketAddress (Ipv4Address::GetAny (), port));
      PacketSinkHelper sinkHelper ("ns3::TcpSocketFactory", sinkLocalAddress);
      ApplicationContainer sinkApp = sinkHelper.Install (n3n4.Get (1));
      sinkApp.Start (Seconds (sink_start_time));
      sinkApp.Stop (Seconds (sink_stop_time));

      // Connection one
      // Clients are in left side
      /*
       * Create the OnOff applications to send TCP to the server
       * onoffhelper is a client that send data to TCP destination
       */
      OnOffHelper clientHelper1 ("ns3::TcpSocketFactory", Address ());
      clientHelper1.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
      clientHelper1.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
      clientHelper1.SetAttribute ("DataRate", DataRateValue (DataRate ("10Mb/s")));
      clientHelper1.SetAttribute ("PacketSize", UintegerValue (500));

      ApplicationContainer clientApps1;
      AddressValue remoteAddress (InetSocketAddress (i3i4.GetAddress (1), port));
      clientHelper1.SetAttribute ("Remote", remoteAddress);
      clientApps1.Add (clientHelper1.Install (n0n2.Get (0)));
      clientApps1.Start (Seconds (client_start_time));
      clientApps1.Stop (Seconds (client_stop_time));
    }
  else if (test == 3)
    {
      // SINK is in the right side
      uint16_t port = 50000;
      Address sinkLocalAddress (InetSocketAddress (Ipv4Address::GetAny (), port));
      PacketSinkHelper sinkHelper ("ns3::TcpSocketFactory", sinkLocalAddress);
      ApplicationContainer sinkApp = sinkHelper.Install (n3n4.Get (1));
      sinkApp.Start (Seconds (sink_start_time));
      sinkApp.Stop (Seconds (sink_stop_time));

      // Connection one
      // Clients are in left side
      /*
       * Create the OnOff applications to send TCP to the server
       * onoffhelper is a client that send data to TCP destination
       */
      OnOffHelper clientHelper1 ("ns3::TcpSocketFactory", Address ());
      clientHelper1.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
      clientHelper1.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
      clientHelper1.SetAttribute ("DataRate", DataRateValue (DataRate ("10Mb/s")));
      clientHelper1.SetAttribute ("PacketSize", UintegerValue (1000));

      ApplicationContainer clientApps1;
      AddressValue remoteAddress (InetSocketAddress (i3i4.GetAddress (1), port));
      clientHelper1.SetAttribute ("Remote", remoteAddress);
      clientApps1.Add (clientHelper1.Install (n0n2.Get (0)));
      clientApps1.Start (Seconds (client_start_time));
      clientApps1.Stop (Seconds (client_stop_time));

      // Connection two
      OnOffHelper clientHelper2 ("ns3::TcpSocketFactory", Address ());
      clientHelper2.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
      clientHelper2.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
      clientHelper2.SetAttribute ("DataRate", DataRateValue (DataRate ("10Mb/s")));
      clientHelper2.SetAttribute ("PacketSize", UintegerValue (1000));

      ApplicationContainer clientApps2;
      clientHelper2.SetAttribute ("Remote", remoteAddress);
      clientApps2.Add (clientHelper2.Install (n1n2.Get (0)));
      clientApps2.Start (Seconds (2.0));
      clientApps2.Stop (Seconds (client_stop_time));

      // Connection three
      OnOffHelper clientHelper3 ("ns3::TcpSocketFactory", Address ());
      clientHelper3.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
      clientHelper3.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
      clientHelper3.SetAttribute ("DataRate", DataRateValue (DataRate ("10Mb/s")));
      clientHelper3.SetAttribute ("PacketSize", UintegerValue (1000));

      ApplicationContainer clientApps3;
      clientHelper3.SetAttribute ("Remote", remoteAddress);
      clientApps3.Add (clientHelper3.Install (n6n2.Get (0)));
      clientApps3.Start (Seconds (4.0));
      clientApps3.Stop (Seconds (client_stop_time));

      // Connection four
      OnOffHelper clientHelper4 ("ns3::TcpSocketFactory", Address ());
      clientHelper4.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
      clientHelper4.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
      clientHelper4.SetAttribute ("DataRate", DataRateValue (DataRate ("10Mb/s")));
      clientHelper4.SetAttribute ("PacketSize", UintegerValue (1000));

      ApplicationContainer clientApps4;
      clientHelper4.SetAttribute ("Remote", remoteAddress);
      clientApps4.Add (clientHelper4.Install (n7n2.Get (0)));
      clientApps4.Start (Seconds (5.0));
      clientApps4.Stop (Seconds (client_stop_time));

      // Connection five
      OnOffHelper clientHelper5 ("ns3::TcpSocketFactory", Address ());
      clientHelper5.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
      clientHelper5.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
      clientHelper5.SetAttribute ("DataRate", DataRateValue (DataRate ("10Mb/s")));
      clientHelper5.SetAttribute ("PacketSize", UintegerValue (1000));

      ApplicationContainer clientApps5;
      clientHelper5.SetAttribute ("Remote", remoteAddress);
      clientApps5.Add (clientHelper5.Install (n8n2.Get (0)));
      clientApps5.Start (Seconds (5.0));
      clientApps5.Stop (Seconds (client_stop_time));
    }
  else // test 4
    {
      // #1
      uint16_t port1 = 50001;
      Address sinkLocalAddress1 (InetSocketAddress (Ipv4Address::GetAny (), port1));
      PacketSinkHelper sinkHelper1 ("ns3::TcpSocketFactory", sinkLocalAddress1);
      ApplicationContainer sinkApp1 = sinkHelper1.Install (n3n4.Get (1));
      sinkApp1.Start (Seconds (sink_start_time));
      sinkApp1.Stop (Seconds (sink_stop_time));
      // #2
      uint16_t port2 = 50002;
      Address sinkLocalAddress2 (InetSocketAddress (Ipv4Address::GetAny (), port2));
      PacketSinkHelper sinkHelper2 ("ns3::TcpSocketFactory", sinkLocalAddress2);
      ApplicationContainer sinkApp2 = sinkHelper2.Install (n3n5.Get (1));
      sinkApp2.Start (Seconds (sink_start_time));
      sinkApp2.Stop (Seconds (sink_stop_time));
      // #3
      uint16_t port3 = 50003;
      Address sinkLocalAddress3 (InetSocketAddress (Ipv4Address::GetAny (), port3));
      PacketSinkHelper sinkHelper3 ("ns3::TcpSocketFactory", sinkLocalAddress3);
      ApplicationContainer sinkApp3 = sinkHelper3.Install (n0n2.Get (0));
      sinkApp3.Start (Seconds (sink_start_time));
      sinkApp3.Stop (Seconds (sink_stop_time));
      // #4
      uint16_t port4 = 50004;
      Address sinkLocalAddress4 (InetSocketAddress (Ipv4Address::GetAny (), port4));
      PacketSinkHelper sinkHelper4 ("ns3::TcpSocketFactory", sinkLocalAddress4);
      ApplicationContainer sinkApp4 = sinkHelper4.Install (n1n2.Get (0));
      sinkApp4.Start (Seconds (sink_start_time));
      sinkApp4.Stop (Seconds (sink_stop_time));

      // Connection #1
      /*
       * Create the OnOff applications to send TCP to the server
       * onoffhelper is a client that send data to TCP destination
       */
      OnOffHelper clientHelper1 ("ns3::TcpSocketFactory", Address ());
      clientHelper1.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
      clientHelper1.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
      clientHelper1.SetAttribute ("DataRate", DataRateValue (DataRate ("10Mb/s")));
      clientHelper1.SetAttribute ("PacketSize", UintegerValue (1000));

      ApplicationContainer clientApps1;
      AddressValue remoteAddress1 (InetSocketAddress (i3i4.GetAddress (1), port1));
      clientHelper1.SetAttribute ("Remote", remoteAddress1);
      clientApps1.Add (clientHelper1.Install (n0n2.Get (0)));
      clientApps1.Start (Seconds (client_start_time));
      clientApps1.Stop (Seconds (client_stop_time));

      // Connection #2
      OnOffHelper clientHelper2 ("ns3::TcpSocketFactory", Address ());
      clientHelper2.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
      clientHelper2.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
      clientHelper2.SetAttribute ("DataRate", DataRateValue (DataRate ("10Mb/s")));
      clientHelper2.SetAttribute ("PacketSize", UintegerValue (1000));

      ApplicationContainer clientApps2;
      AddressValue remoteAddress2 (InetSocketAddress (i3i5.GetAddress (1), port2));
      clientHelper2.SetAttribute ("Remote", remoteAddress2);
      clientApps2.Add (clientHelper2.Install (n1n2.Get (0)));
      clientApps2.Start (Seconds (2.0));
      clientApps2.Stop (Seconds (client_stop_time));

      // Connection #3
      OnOffHelper clientHelper3 ("ns3::TcpSocketFactory", Address ());
      clientHelper3.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
      clientHelper3.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
      clientHelper3.SetAttribute ("DataRate", DataRateValue (DataRate ("10Mb/s")));
      clientHelper3.SetAttribute ("PacketSize", UintegerValue (1000));

      ApplicationContainer clientApps3;
      AddressValue remoteAddress3 (InetSocketAddress (i0i2.GetAddress (0), port3));
      clientHelper3.SetAttribute ("Remote", remoteAddress3);
      clientApps3.Add (clientHelper3.Install (n3n4.Get (1)));
      clientApps3.Start (Seconds (3.5));
      clientApps3.Stop (Seconds (client_stop_time));

      // Connection #4
      OnOffHelper clientHelper4 ("ns3::TcpSocketFactory", Address ());
      clientHelper4.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
      clientHelper4.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
      clientHelper4.SetAttribute ("DataRate", DataRateValue (DataRate ("40b/s")));
      clientHelper4.SetAttribute ("PacketSize", UintegerValue (5 * 8)); // telnet

      ApplicationContainer clientApps4;
      AddressValue remoteAddress4 (InetSocketAddress (i1i2.GetAddress (0), port4));
      clientHelper4.SetAttribute ("Remote", remoteAddress4);
      clientApps4.Add (clientHelper4.Install (n3n5.Get (1)));
      clientApps4.Start (Seconds (1.0));
      clientApps4.Stop (Seconds (client_stop_time));
    }
}

int
main (int argc, char *argv[])
{
  LogComponentEnable ("PieQueue", LOG_LEVEL_INFO);

  uint32_t pieTest;
  std::string pieLinkDataRate = "10Mbps";
  std::string pieLinkDelay = "50ms";

  std::string pathOut;
  bool writeForPlot = true;
  bool writePcap = false;
  bool flowMonitor = false;

  bool printPieStats = true;

  global_start_time = 0.0;
  global_stop_time = 100.0;
  sink_start_time = global_start_time;
  sink_stop_time = global_stop_time + 3;
  client_start_time = sink_start_time + 0.2;
  client_stop_time = global_stop_time - 2.;

  // Configuration and command line parameter parsing
  pieTest = 3;
  // Will only save in the directory if enable opts below
  pathOut = "."; // Current directory
  CommandLine cmd;
  cmd.AddValue ("pieLinkDataRate", "Data rate of link", pieLinkDataRate);
  cmd.AddValue ("pieLinkDelay", "delay of link", pieLinkDelay);
  cmd.AddValue ("testNumber", "Run test 1, 3, and 4", pieTest);
  cmd.AddValue ("pathOut", "Path to save results from --writeForPlot/--writePcap/--writeFlowMonitor", pathOut);
  cmd.AddValue ("writeForPlot", "<0/1> to write results for plot (gnuplot)", writeForPlot);
  cmd.AddValue ("writePcap", "<0/1> to write results in pcapfile", writePcap);
  cmd.AddValue ("writeFlowMonitor", "<0/1> to enable Flow Monitor and write their results", flowMonitor);

  cmd.Parse (argc, argv);

  if ( (pieTest != 1) && (pieTest != 3) && (pieTest != 4))
    {
      NS_ABORT_MSG ("Invalid test number. Supported tests are 1, 3, and 4");
    }

  NS_LOG_INFO ("Create nodes");
  NodeContainer c;
  c.Create (9);
  Names::Add ( "N0", c.Get (0));
  Names::Add ( "N1", c.Get (1));
  Names::Add ( "N2", c.Get (2));
  Names::Add ( "N3", c.Get (3));
  Names::Add ( "N4", c.Get (4));
  Names::Add ( "N5", c.Get (5));
  Names::Add ( "N6", c.Get (6));
  Names::Add ( "N7", c.Get (7));
  Names::Add ( "N8", c.Get (8));
  n0n2 = NodeContainer (c.Get (0), c.Get (2));
  n1n2 = NodeContainer (c.Get (1), c.Get (2));
  n2n3 = NodeContainer (c.Get (2), c.Get (3));
  n3n4 = NodeContainer (c.Get (3), c.Get (4));
  n3n5 = NodeContainer (c.Get (3), c.Get (5));
  n6n2 = NodeContainer (c.Get (6), c.Get (2));
  n7n2 = NodeContainer (c.Get (7), c.Get (2));
  n8n2 = NodeContainer (c.Get (8), c.Get (2));

  Config::SetDefault ("ns3::TcpL4Protocol::SocketType", StringValue ("ns3::TcpReno"));
  Config::SetDefault ("ns3::TcpSocket::SegmentSize", UintegerValue (1000 - 42));
  Config::SetDefault ("ns3::TcpSocket::DelAckCount", UintegerValue (1));
  GlobalValue::Bind ("ChecksumEnabled", BooleanValue (false));

  uint32_t meanPktSize = 1000;

  // PIE params
  NS_LOG_INFO ("Set PIE params");

  Config::SetDefault ("ns3::PieQueue::MeanPktSize", UintegerValue (meanPktSize));
  Config::SetDefault ("ns3::PieQueue::QueueDelayReference", TimeValue (Seconds (0.002)));
  Config::SetDefault ("ns3::PieQueue::A", DoubleValue (0.125));
  Config::SetDefault ("ns3::PieQueue::B", DoubleValue (1.25));

  NS_LOG_INFO ("Install internet stack on all nodes.");
  InternetStackHelper internet;
  internet.Install (c);

  NS_LOG_INFO ("Create channels");
  PointToPointHelper p2p;

  p2p.SetQueue ("ns3::DropTailQueue");
  p2p.SetDeviceAttribute ("DataRate", StringValue ("10Mbps"));
  p2p.SetChannelAttribute ("Delay", StringValue ("2ms"));
  NetDeviceContainer devn0n2 = p2p.Install (n0n2);

  p2p.SetQueue ("ns3::DropTailQueue");
  p2p.SetDeviceAttribute ("DataRate", StringValue ("10Mbps"));
  p2p.SetChannelAttribute ("Delay", StringValue ("3ms"));
  NetDeviceContainer devn1n2 = p2p.Install (n1n2);

  p2p.SetQueue ("ns3::DropTailQueue");
  p2p.SetDeviceAttribute ("DataRate", StringValue ("10Mbps"));
  p2p.SetChannelAttribute ("Delay", StringValue ("3ms"));
  NetDeviceContainer devn6n2 = p2p.Install (n6n2);

  p2p.SetQueue ("ns3::DropTailQueue");
  p2p.SetDeviceAttribute ("DataRate", StringValue ("10Mbps"));
  p2p.SetChannelAttribute ("Delay", StringValue ("3ms"));
  NetDeviceContainer devn7n2 = p2p.Install (n7n2);

  p2p.SetQueue ("ns3::DropTailQueue");
  p2p.SetDeviceAttribute ("DataRate", StringValue ("10Mbps"));
  p2p.SetChannelAttribute ("Delay", StringValue ("5ms"));
  NetDeviceContainer devn8n2 = p2p.Install (n8n2);

  p2p.SetQueue ("ns3::PieQueue", // only backbone link has PIE queue
                "LinkBandwidth", StringValue (pieLinkDataRate),
                "LinkDelay", StringValue (pieLinkDelay));
  p2p.SetDeviceAttribute ("DataRate", StringValue (pieLinkDataRate));
  p2p.SetChannelAttribute ("Delay", StringValue (pieLinkDelay));
  NetDeviceContainer devn2n3 = p2p.Install (n2n3);

  p2p.SetQueue ("ns3::DropTailQueue");
  p2p.SetDeviceAttribute ("DataRate", StringValue ("10Mbps"));
  p2p.SetChannelAttribute ("Delay", StringValue ("4ms"));
  NetDeviceContainer devn3n4 = p2p.Install (n3n4);

  p2p.SetQueue ("ns3::DropTailQueue");
  p2p.SetDeviceAttribute ("DataRate", StringValue ("10Mbps"));
  p2p.SetChannelAttribute ("Delay", StringValue ("5ms"));
  NetDeviceContainer devn3n5 = p2p.Install (n3n5);

  NS_LOG_INFO ("Assign IP Addresses");
  Ipv4AddressHelper ipv4;

  ipv4.SetBase ("10.1.1.0", "255.255.255.0");
  i0i2 = ipv4.Assign (devn0n2);

  ipv4.SetBase ("10.1.2.0", "255.255.255.0");
  i1i2 = ipv4.Assign (devn1n2);

  ipv4.SetBase ("10.1.6.0", "255.255.255.0");
  i6i2 = ipv4.Assign (devn6n2);

  ipv4.SetBase ("10.1.7.0", "255.255.255.0");
  i7i2 = ipv4.Assign (devn7n2);

  ipv4.SetBase ("10.1.8.0", "255.255.255.0");
  i8i2 = ipv4.Assign (devn8n2);

  ipv4.SetBase ("10.1.3.0", "255.255.255.0");
  i2i3 = ipv4.Assign (devn2n3);

  ipv4.SetBase ("10.1.4.0", "255.255.255.0");
  i3i4 = ipv4.Assign (devn3n4);

  ipv4.SetBase ("10.1.5.0", "255.255.255.0");
  i3i5 = ipv4.Assign (devn3n5);

  // Set up the routing
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  BuildAppsTest (pieTest);
  AsciiTraceHelper ascii;
  p2p.EnableAsciiAll (ascii.CreateFileStream ("pietrace.tr"));
  p2p.EnablePcapAll ("piequeue", true);

  if (writePcap)
    {
      PointToPointHelper ptp;
      std::stringstream stmp;
      stmp << pathOut << "/pie";
      ptp.EnablePcapAll (stmp.str ().c_str ());
    }

  Ptr<FlowMonitor> flowmon;
  if (flowMonitor)
    {
      FlowMonitorHelper flowmonHelper;
      flowmon = flowmonHelper.InstallAll ();
    }

  if (writeForPlot)
    {
      filePlotQueue << pathOut << "/" << "pie-queue.plotme";
      filePlotQueueAvg << pathOut << "/" << "pie-queue_avg.plotme";
      filePlotQueueNoDrop << pathOut << "/" << "number of drop.plotme";
      filePlotQueueDropProb << pathOut << "/" << "drop_prob.plotme";
      filePlotQueueDel << pathOut << "/" << "delay.plotme";

      remove (filePlotQueueDel.str ().c_str ());
      remove (filePlotQueue.str ().c_str ());
      remove (filePlotQueueAvg.str ().c_str ());
      remove (filePlotQueueNoDrop.str ().c_str ());
      remove (filePlotQueueDropProb.str ().c_str ());
      Ptr<PointToPointNetDevice> nd = StaticCast<PointToPointNetDevice> (devn2n3.Get (0));
      Ptr<Queue> queue = nd->GetQueue ();
      Simulator::ScheduleNow (&CheckQueueSize, queue);
      Simulator::ScheduleNow (&CheckQueueDel, queue);
      Simulator::ScheduleNow (&CheckQueueProb, queue);
    }

  Simulator::Stop (Seconds (sink_stop_time));
  Simulator::Run ();

  if (flowMonitor)
    {
      std::stringstream stmp;
      stmp << pathOut << "/pie.flowmon";

      flowmon->SerializeToXmlFile (stmp.str ().c_str (), false, false);
    }

  if (printPieStats)
    {
      Ptr<PointToPointNetDevice> nd = StaticCast<PointToPointNetDevice> (devn2n3.Get (0));
      PieQueue::Stats st = StaticCast<PieQueue> (nd->GetQueue ())->GetStats ();
      std::cout << "*** pie stats from Node 2 queue ***" << std::endl;
      std::cout << "\t " << st.unforcedDrop << " drops due to probability " << std::endl;
      std::cout << "\t " << st.forcedDrop << " drops due queue full" << std::endl;

      nd = StaticCast<PointToPointNetDevice> (devn2n3.Get (1));
      st = StaticCast<PieQueue> (nd->GetQueue ())->GetStats ();
      std::cout << "*** pie stats from Node 3 queue ***" << std::endl;
      std::cout << "\t " << st.unforcedDrop << " drops due to probability" << std::endl;
      std::cout << "\t " << st.forcedDrop << " drops due queue full" << std::endl;
    }

  Simulator::Destroy ();

  return 0;
}
