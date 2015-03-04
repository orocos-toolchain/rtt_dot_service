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
    ,m_comp_args("style=filled,width=1.8,height=1.8,")
    ,m_conn_args(" ")
    ,m_chan_args("shape=box,")
{
    this->addOperation("getOwnerName", &Dot::getOwnerName, this).doc("Returns the name of the owner of this object.");
    this->addOperation("generate", &Dot::execute, this).doc("Generate component overview and write to 'dot_file'.");
    this->addProperty("dot_file", m_dot_file).doc("File to write the generated dot syntax to.");
    this->addProperty("comp_args", m_comp_args).doc("Arguments to add to the component drawings.");
    this->addProperty("conn_args", m_conn_args).doc("Arguments to add to the connection drawings.");
    this->addProperty("chan_args", m_chan_args).doc("Arguments to add to the channel drawings.");
    this->doc("Dot service interface.");
    owner->engine()->runFunction(this);
}

std::string Dot::getOwnerName() {
    return getOwner()->getName();
}

std::string Dot::quote(std::string const& name){
  return "\"" + name + "\"";
}

void Dot::scanService(std::string path, Service::shared_ptr sv)
{
    std::vector<std::string> comp_ports;
    comp_ports.clear();
    // Get all component ports
    comp_ports = sv->getPortNames();
    // Loop over all ports
    for(unsigned int j = 0; j < comp_ports.size(); j++){
      log(Debug) << "Port: " << comp_ports[j] << endlog();
      std::list<internal::ConnectionManager::ChannelDescriptor> chns = sv->getPort(comp_ports[j])->getManager()->getChannels();
      std::list<internal::ConnectionManager::ChannelDescriptor>::iterator k;
      if(chns.empty()){
        log(Debug) << "Looks like we have an empty channel!" << endlog();
        // Display unconnected ports as well!
        m_dot << quote(comp_ports[j]) << "[shape=point];\n";
        if(dynamic_cast<base::InputPortInterface*>(sv->getPort(comp_ports[j])) != 0){
          m_dot << quote(comp_ports[j]) << "->" << mpeer << "[" << m_conn_args << "label=" << quote(comp_ports[j]) << "];\n";
        }
        else{
          m_dot << mpeer << "->" << quote(comp_ports[j]) << "[" << m_conn_args << "label=" << quote(comp_ports[j]) << "];\n";
        }
      }
      for(k = chns.begin(); k != chns.end(); k++){
        base::ChannelElementBase::shared_ptr bs = k->get<1>();
        ConnPolicy cp = k->get<2>();
        log(Debug) << "Connection id: " << cp.name_id << endlog();
        std::string comp_in, port_in;
        if(bs->getInputEndPoint()->getPort() != 0){
          if (bs->getInputEndPoint()->getPort()->getInterface() != 0 ){
            comp_in = bs->getInputEndPoint()->getPort()->getInterface()->getOwner()->getName();
          }
          else{
            comp_in = "free input ports";
          }
          port_in = bs->getInputEndPoint()->getPort()->getName();
        }
        log(Debug) << "Connection starts at port: " << port_in << endlog();
        log(Debug) << "Connection starts at component: " << comp_in << endlog();
        std::string comp_out, port_out;
        if(bs->getOutputEndPoint()->getPort() != 0){
          if (bs->getOutputEndPoint()->getPort()->getInterface() != 0 ){
            comp_out = bs->getOutputEndPoint()->getPort()->getInterface()->getOwner()->getName();
          }
          else{
            comp_out = "free output ports";
          }
          port_out = bs->getOutputEndPoint()->getPort()->getName();
        }
        log(Debug) << "Connection ends at port: " << port_out << endlog();
        log(Debug) << "Connection ends at component: " << comp_out << endlog();
        std::string conn_info;
        std::stringstream ss;
        switch(cp.type){
        case ConnPolicy::DATA:
            conn_info = "data";
            break;
        case ConnPolicy::BUFFER:
            ss << "buffer [ " << cp.size << " ]";
            conn_info = ss.str();
            break;
        case ConnPolicy::CIRCULAR_BUFFER:
            ss << "circbuffer [ " << cp.size << " ]";
            conn_info = ss.str();
            break;
        default:
            conn_info = "unknown";
        }
        log(Debug) << "Connection has conn_info: " << conn_info << endlog();
        // Only consider input ports
        if(dynamic_cast<base::InputPortInterface*>(sv->getPort(comp_ports[j])) != 0){
          // First, consider regular connections
          if(!comp_in.empty()){
            // If the ConnPolicy has a non-empty name, use that name as the topic name
            if(!cp.name_id.empty()){
              // plot the channel element as a seperate box and connect input and output with it
              m_dot << quote(cp.name_id) << "[" << m_chan_args << "label=" << quote(cp.name_id) << "];\n";
              m_dot << quote(comp_in) << "->" << quote(cp.name_id) << "[" << m_conn_args << "label=" << quote(port_in) << "];\n";
              m_dot << quote(cp.name_id) << "->" << quote(comp_out) << "[" << m_conn_args << "label=" << quote(port_out) << "];\n";
            }
            // Else, use a custom name: compInportIncompOutportOut
            else{
              // plot the channel element as a seperate box and connect input and output with it
              m_dot << quote(comp_in + port_in + comp_out + port_out) << "[" << m_chan_args << "label=" << quote(conn_info) << "];\n";
              m_dot << quote(comp_in) << "->" << quote(comp_in + port_in + comp_out + port_out) << "[" << m_conn_args << "label=" << quote(port_in) << "];\n";
              m_dot << quote(comp_in + port_in + comp_out + port_out) << "->" << comp_out << "[" << m_conn_args << "label=" << quote(port_out) << "];\n";
            }
          }
          // Here, we have a stream?!
          else{
            m_dot << quote(comp_out + port_out) << "[" << m_chan_args << "label=" << quote(conn_info) << "];\n";
            m_dot << quote(comp_out + port_out) << "->" << quote(comp_out) << "[" << m_conn_args << "label=" << quote(port_out) << "];\n";
          }
        }
        else{
          // Consider only output ports that do not have a corresponding input port
          if(comp_out.empty()){
            // If the ConnPolicy has a non-empty name, use that name as the topic name
            if(!cp.name_id.empty()){
              // plot the channel element as a seperate box and connect input and output with it
              m_dot << quote(cp.name_id) << "[" << m_chan_args << "label=" << quote(cp.name_id) << "];\n";
              m_dot << quote(comp_in) << "->" << quote(cp.name_id) << "[" << m_conn_args << "label=" << quote(port_in) << "];\n";
            }
            else{
              // plot the channel element as a seperate box and connect input and output with it
              m_dot << quote(comp_in + port_in) << "[" << m_chan_args << "label=" << quote(conn_info) << "];\n";
              m_dot << quote(comp_in) << "->" << quote( comp_in + port_in) << "[" << m_conn_args << "label=" << quote(port_in) << "];\n";
            }
          }
          else {
              log(Debug) << "Dropped " << conn_info << " from " << comp_out + port_out << " to " << comp_in + port_in << endlog();
          }
        }
      }
    }
    // Recurse:
    Service::ProviderNames providers = sv->getProviderNames();
    for(Service::ProviderNames::iterator it=providers.begin(); it != providers.end(); ++it) {
        scanService(path + "." + sv->getName(), sv->provides(*it) );
    }
}


bool Dot::execute(){
  m_dot.str("");
  m_dot << "digraph G { \n";
  m_dot << "rankdir=TB; \n";
  // List all peers of this component
  std::vector<std::string> peerList = this->getOwner()->getPeerList();
  // Add the component itself as well
  peerList.push_back(this->getOwner()->getName());
  if(peerList.size() == 0){
    log(Debug) << "Component has no peers!" << endlog();
  }
  // Loop over all peers + own component
  for(unsigned int i = 0; i < peerList.size(); i++){
    mpeer = peerList[i];
    log(Debug) << "Component: " << mpeer << endlog();
    // Get a pointer to the taskcontext, which can be either a peer or the component itself.
    TaskContext* tc;
    if(this->getOwner()->getPeer(mpeer) == 0){
      tc = this->getOwner();
    }
    else{
      tc = this->getOwner()->getPeer(mpeer);
    }
    // Get the component state
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
    // Draw component with color according to its TaskState
    m_dot << quote(mpeer) << "[" << m_comp_args << "color=" << color << "];\n";
    // Draw in/out ports
    scanService("",tc->provides());
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
