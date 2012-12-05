
#pragma once

#include <cstdio>
#include <vector>
#include <fstream>
#include <map>
using namespace std;

#include <xercesc/dom/DOM.hpp>
#include <xercesc/dom/DOMDocument.hpp>
#include <xercesc/dom/DOMElement.hpp>
#include <xercesc/dom/DOMNodeList.hpp>
#include <xercesc/parsers/XercesDOMParser.hpp>

#include <boost/lexical_cast.hpp>
using namespace std;
using namespace xercesc;
using boost::lexical_cast;
using boost::bad_lexical_cast;

class MyException
{
	public:
		MyException()
    {
		}
		MyException(string error_message)
    {
			m_error_message = error_message;
		}
		string m_error_message;
};
class ConfigParser
{
	public:
		ConfigParser(){ }
		virtual bool parse()=0;
		void setInputfile(string& config_file)
    {
      ifstream ifile (config_file.c_str ());
      if (!ifile) {
        throw MyException ("Input file doesn't exist");
      }
      ifile.close ();
			m_config_file = config_file;
		}
			
		void setErrorMessage(string error_message)
    {
			m_error_message = error_message;
		}

		const char* getInputfile() const { return m_config_file.c_str(); }
		const char* getErrorMessage() const { return m_error_message.c_str(); }

    map <int,int>& getFilterWheelById (int id) 
    { 
      return filters[id];
    }

    string& get_fw_port (int id) 
    { 
      return fw_ports[id]; 
    }

    int& get_fw_baud (int id)
    {
      return fw_bauds[id];
    }


	private:

		string m_config_file;
		string m_error_message;

  protected:
    map <int, int> filters[2];
    string fw_ports[2];
    int fw_bauds[2];
};
class ConfigParserDOM : public ConfigParser
{
	public:
		ConfigParserDOM()
    { 
			try{
				XMLPlatformUtils::Initialize();
			}
			catch(XMLException &e){
				char *message = XMLString::transcode(e.getMessage());
				cerr << "XML toolkit initialization error : " << message << endl;
				XMLString::release(&message);
			}
			attr_exposure = XMLString::transcode("duration");
			attr_slot = XMLString::transcode("slot");
			attr_wavelength = XMLString::transcode("wavelength");
			attr_port = XMLString::transcode("port");

			m_parser = new XercesDOMParser();
		}
		bool parse();

		~ConfigParserDOM()
    {
			delete m_parser;
			try{
				XMLPlatformUtils::Terminate();
			}
			catch(XMLException& e){
				char *message = XMLString::transcode(e.getMessage());
				cerr << "XML toolkit shutdown error : " << message << endl;
				XMLString::release(&message);
			}
		}
	private:
    bool getFilterWheelConfig (DOMNode *node, int id);
		bool getExposureSettings(DOMNode *node);
		xercesc::XercesDOMParser *m_parser;

		XMLCh *attr_exposure;
		XMLCh *attr_slot;
		XMLCh *attr_port;
		XMLCh *attr_wavelength;
};

bool ConfigParserDOM::parse()
{
	bool status = true;
	try{
		m_parser->setValidationScheme(XercesDOMParser::Val_Never);
		m_parser->setDoNamespaces(false);
		m_parser->setDoSchema(false);
		m_parser->setLoadExternalDTD(false);

		cout << "Input file is " << getInputfile() << endl;
		m_parser->parse(getInputfile());

		DOMDocument *xml_doc = m_parser->getDocument();

		DOMElement *ir_camera = xml_doc->getDocumentElement();
		if(!ir_camera){
			status = false;
			setErrorMessage ("Empty XML document");
			return status;
		}

		const XMLCh *tag_name = ir_camera->getTagName();
		char *str = XMLString::transcode(tag_name);

		cout << "Root name = " << str  << endl;

		DOMNodeList *children = ir_camera->getChildNodes();

    int filter_wheel_id = 0;
		for(int i=0; i<children->getLength(); i++) {
			DOMNode *item = children->item(i);
			if(item->getNodeType() != DOMNode::ELEMENT_NODE) continue;
			const XMLCh *tag = item->getNodeName();
			const char* tag_name = XMLString::transcode(tag);
			cout <<"tag name = " << tag_name << endl;
			if (strcmp (tag_name, "FilterWheel") == 0) {
        getFilterWheelConfig (item, filter_wheel_id++);
      }
			if (strcmp (tag_name, "ExposureSettings") == 0) {
        //getExposureSettings (item);
      }
		}
	}
	catch(XMLException &e) {
		char *message = XMLString::transcode(e.getMessage());
		cerr << "XML parser error : " << message << endl;
		XMLString::release(&message);
		status = false;
	}
	return status;
}
bool ConfigParserDOM::getFilterWheelConfig (DOMNode *node, int id)
{
  assert (id < 2 && "               <<< Only two filter wheels should be configured in the xml file.. Please check.. >>> ");

  DOMNamedNodeMap* nodemap = node->getAttributes ();
  for (XMLSize_t i = 0; i < nodemap->getLength (); i++) {
    DOMNode* named_node = nodemap->item (i);
    char* name = NULL;
    char* value = NULL;
    cout << (name = XMLString::transcode (named_node->getNodeName ())) << "\t"
          << (value = XMLString::transcode (named_node->getNodeValue ())) << endl;
    if (strcmp (name, "port") == 0) {
      get_fw_port (id) = value;
    }
    if (strcmp (name, "baudrate") == 0) {
      get_fw_baud (id) = boost::lexical_cast <int> (value);
    }
  }

  bool status = true;
	DOMNodeList *children = node->getChildNodes();
  char error_message[1024];

  int counter = 1;
	for(int i = 0; i < children->getLength(); i++) {
    int slot = 0;
    int wavelength = 0;
		DOMNode *item = children->item(i);
		if(item->getNodeType() != DOMNode::ELEMENT_NODE) continue;
		
		const XMLCh *tag = item->getNodeName();
		const char *tag_name = XMLString::transcode(tag);

		if(strcmp(tag_name, "filter") != 0) {
			sprintf(error_message,"<filter> node missing at position %d in element <ExposureSettings>", counter);
			throw MyException(error_message); 
		}

    // slot attribute
		DOMElement *element = dynamic_cast<DOMElement*>(item);
		const XMLCh *slot_attr_element = element->getAttribute (attr_slot);

		const char *s = XMLString::transcode (slot_attr_element); 
		if (strlen (s)==0) {
			sprintf(error_message, "slot attribute missing at <filter> node # %d in <FilterWheel>", counter);
			throw MyException (error_message);
		}
		try {
			double val = lexical_cast<double>(s);
			cout << "\tSlot = " << val;
      slot = val;
		}
		catch(bad_lexical_cast& e){
			sprintf(error_message, "Invalid value for slot at node # %d in <ExposureSettings>", counter);
			throw MyException(error_message);
		}

    // wavelength attribute
		const XMLCh *wavelength_attr_element = element->getAttribute (attr_wavelength);

		s = XMLString::transcode (wavelength_attr_element); 
		if (strlen (s)==0) {
			sprintf(error_message, "wavelength attribute missing at <filter> node # %d in <FilterWheel>", counter);
			throw MyException (error_message);
		}
		try {
			double val = lexical_cast<double>(s);
			cout << "\tWavelength = " << val << endl;
      wavelength = val;
		}
		catch(bad_lexical_cast& e){
			sprintf(error_message, "Invalid value for slot at node # %d in <ExposureSettings>", counter);
			throw MyException(error_message);
		}
    getFilterWheelById (id)[slot] = wavelength;
    counter++;
  }
  cout << "Number of filters = " << getFilterWheelById (id).size () << endl;
  return status;
}
bool ConfigParserDOM::getExposureSettings(DOMNode *node)
{
	cout << "Node = ExposureSettings\n";
	
	DOMNodeList *children = node->getChildNodes();

	int counter = 1;
	char error_message[1024];

	for(int i=0; i<children->getLength(); i++) {
		DOMNode *item = children->item(i);
		if(item->getNodeType() != DOMNode::ELEMENT_NODE) continue;
		
		const XMLCh *tag = item->getNodeName();
		const char *tag_name = XMLString::transcode(tag);

		if(strcmp(tag_name, "filter") != 0){
			sprintf(error_message,"<filter> node missing at position %d in element <ExposureSettings>", counter);
			throw MyException(error_message); 
		}

		DOMElement *element = dynamic_cast<DOMElement*>(item);
		const XMLCh *exposure_attr_element = element->getAttribute(attr_exposure);

		const char *s = XMLString::transcode(exposure_attr_element); 
		if(strlen(s)==0){
			sprintf(error_message, "duration attribute missing at <filter> node # %d in <ExposureSettings>", counter);
			throw MyException(error_message);
		}
		try{
			double val = lexical_cast<double>(s);
			cout << "\tValue = " << val << endl;
		}
		catch(bad_lexical_cast& e){
			sprintf(error_message, "Invalid value for duration at node # %d in <ExposureSettings>", counter);
			throw MyException(error_message);
		}
		counter++;
	}
}
