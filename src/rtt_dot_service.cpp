/******************************************************************************
*                           OROCOS dot service                                *
*                                                                             *
*                         (C) 2011 Steven Bellens                             *
*                     steven.bellens@mech.kuleuven.be                         *
*                    Department of Mechanical Engineering,                    *
*                   Katholieke Universiteit Leuven, Belgium.                  *
*                                                                             *
*       You may redistribute this software and/or modify it under either the  *
*       terms of the GNU Lesser General Public License version 2.1 (LGPLv2.1  *
*       <http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html>) or (at your *
*       discretion) of the Modified BSD License:                              *
*       Redistribution and use in source and binary forms, with or without    *
*       modification, are permitted provided that the following conditions    *
*       are met:                                                              *
*       1. Redistributions of source code must retain the above copyright     *
*       notice, this list of conditions and the following disclaimer.         *
*       2. Redistributions in binary form must reproduce the above copyright  *
*       notice, this list of conditions and the following disclaimer in the   *
*       documentation and/or other materials provided with the distribution.  *
*       3. The name of the author may not be used to endorse or promote       *
*       products derived from this software without specific prior written    *
*       permission.                                                           *
*       THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR  *
*       IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED        *
*       WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE    *
*       ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,*
*       INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES    *
*       (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS       *
*       OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) *
*       HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,   *
*       STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING *
*       IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE    *
*       POSSIBILITY OF SUCH DAMAGE.                                           *
*                                                                             *
*******************************************************************************/

#include "rtt_dot_service.hpp"
#include <boost/algorithm/string.hpp>
#include <fstream>

Dot::Dot(TaskContext* owner)
    : Service("dot", owner), base::ExecutableInterface()
    ,m_dot_file("orograph.dot")
{
    this->addOperation("getOwnerName", &Dot::getOwnerName, this).doc("Returns the name of the owner of this object.");
    this->addOperation("generate", &Dot::execute, this).doc("Generate component overview and write to 'dot_file'.");
    this->addProperty("dot_file", m_dot_file).doc("File to write the generated dot syntax to.");
    this->doc("Dot service interface.");
    owner->engine()->runFunction(this);
}

std::string Dot::getOwnerName() {
    return getOwner()->getName();
}

bool Dot::execute(){
  m_dot.str("");
  m_dot << "digraph G { \n";
  m_dot << "rankdir=TB; \n";
  std::vector<std::string> peerList = this->getOwner()->getPeerList();
  peerList.push_back(this->getOwner()->getName());
  if(peerList.size() == 0){
    log(Debug) << "Component has no peers!" << endlog();
  }
  std::vector<std::string> comp_ports;
  for(unsigned int i=0; i<peerList.size(); i++){
    log(Debug) << "Component: " << peerList[i] << endlog();
    // Get a pointer to the taskcontext, which can be either a peer or the component itself.
    TaskContext* tc;
    if(this->getOwner()->getPeer(peerList[i])==0){
      tc = this->getOwner();
    } else{
      tc = this->getOwner()->getPeer(peerList[i]);
    }
    base::TaskCore::TaskState st;
    st = tc->getTaskState();
    log(Debug) << "This component has state: " << st << endlog();
    std::string color;
    switch (st){
      case 0: color = "white"; break;
      case 1: color = "orange"; break;
      case 2: color = "red"; break;
      case 3: color = "red"; break;
      case 4: color = "lightblue"; break;
      case 5: color = "green"; break;
      case 6: color = "red"; break;
    }
    m_dot << peerList[i] << "[style=filled,color=" << color << "];\n";
    comp_ports.clear();
    comp_ports = tc->ports()->getPortNames();
    for(unsigned int j=0; j < comp_ports.size(); j++){
      log(Debug) << "Ports: " << comp_ports[j] << endlog();
      // Only consider input ports
      if(dynamic_cast<base::InputPortInterface*>(tc->getPort(comp_ports[j])) != 0){
        std::list<RTT::internal::ConnectionManager::ChannelDescriptor> chns = tc->getPort(comp_ports[j])->getManager()->getChannels();
        std::list<RTT::internal::ConnectionManager::ChannelDescriptor>::iterator k;
        for(k=chns.begin(); k!= chns.end(); k++){
          base::ChannelElementBase::shared_ptr bs = k->get<1>();
          ConnPolicy cp = k->get<2>();
          std::string comp_in, port_in;
          if(bs->getInputEndPoint()->getPort()!=0){
            comp_in = bs->getInputEndPoint()->getPort()->getInterface()->getOwner()->getName();
            port_in = bs->getInputEndPoint()->getPort()->getName();
          }
          log(Debug) << "Connection starts at: " << port_in << endlog();
          std::string comp_out, port_out;
          if(bs->getOutputEndPoint()->getPort()!=0){
            comp_out = bs->getOutputEndPoint()->getPort()->getInterface()->getOwner()->getName();
            port_out = bs->getOutputEndPoint()->getPort()->getName();
          }
          log(Debug) << "Connection ends at: " << port_out << endlog();
          // If the ConnPolicy has a non-empty name, use that name as the topic name
          if(!cp.name_id.empty()){
            std::string cp_out = cp.name_id;
            // DOT does not handle "/" well, so we replace them with "_" here
            boost::replace_all(cp_out,"/","_");
            // plot the channel element as a seperate box and connect input and output with it
            m_dot << cp_out << "[shape=box];\n";
            m_dot << comp_in << "->" << cp_out << "[label=\"" << port_in << "\"];\n";
            m_dot << cp_out << "->" << comp_out << "[label=\"" << port_out << "\"];\n";\
          }
          // Else, use a custom name: compInName.compOutName
          else if(!comp_in.empty() && !comp_out.empty()){
            // plot the channel element as a seperate box and connect input and output with it
            m_dot << "\"" << comp_in << "." << comp_out << "\"[shape=box];\n";
            m_dot << comp_in << "->" << "\"" << comp_in << "." << comp_out << "\"[label=\"" << port_in << "\"];\n";
            m_dot << "\"" << comp_in << "." << comp_out << "\"->" << comp_out << "[label=\"" << port_out << "\"];\n";\
          }
        }
      }
      // Consider output ports that do not have a corresponding input port
      else{
        std::list<RTT::internal::ConnectionManager::ChannelDescriptor> chns = tc->getPort(comp_ports[j])->getManager()->getChannels();
        std::list<RTT::internal::ConnectionManager::ChannelDescriptor>::iterator k;
        for(k=chns.begin(); k!= chns.end(); k++){
          base::ChannelElementBase::shared_ptr bs = k->get<1>();
          ConnPolicy cp = k->get<2>();
          std::string comp_in, port_in;
          if(bs->getInputEndPoint()->getPort()!=0){
            comp_in = bs->getInputEndPoint()->getPort()->getInterface()->getOwner()->getName();
            port_in = bs->getInputEndPoint()->getPort()->getName();
          }
          log(Debug) << "Connection starts at: " << port_in << endlog();
          std::string comp_out, port_out;
          if(bs->getOutputEndPoint()->getPort()!=0){
            comp_out = bs->getOutputEndPoint()->getPort()->getInterface()->getOwner()->getName();
            port_out = bs->getOutputEndPoint()->getPort()->getName();
          }
          log(Debug) << "Connection ends at: " << port_out << endlog();
          // If the ConnPolicy has a non-empty name, use that name as the topic name
          if(!cp.name_id.empty()){
            std::string cp_out = cp.name_id;
            // DOT does not handle "/" well, so we replace them with "_" here
            boost::replace_all(cp_out,"/","_");
            // plot the channel element as a seperate box and connect input and output with it
            m_dot << cp_out << "[shape=box];\n";
            m_dot << comp_in << "->" << cp_out << "[label=\"" << port_in << "\"];\n";
          }
          // Else, use a custom name: compInName.compOutName
          else if(!comp_in.empty() && comp_out.empty()){
            // plot the channel element as a seperate box and connect input and output with it
            m_dot << "\"" << comp_in << "." << comp_out << "\"[shape=box];\n";
            m_dot << comp_in << "->" << "\"" << comp_in << "." << comp_out << "\"[label=\"" << port_in << "\"];\n";
          }
        }
      }
    }
  }
  m_dot << "}\n";
  std::ofstream fl(m_dot_file.c_str());
  if (fl.is_open()){
    fl << m_dot.str();
    fl.close();
    return true;
  }
  else{
    log(Warning) << "Unable to open file: " << m_dot_file << endlog();
    return false;
  }
}
ORO_SERVICE_NAMED_PLUGIN(Dot, "dot")
