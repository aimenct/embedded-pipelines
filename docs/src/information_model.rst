Information Model
=================

Embeded Pipelines information model main purpose is to describe the raw data that goes through the queues, and allow readers and writers to correctly interpret them. Besides, it also serve to describe and organise filter settings. Its flexibility also allows to represent even complex hierarchical models like GeniCam XML description files, Asset Administration Shell (AAS) or OPC-UA server nodes.

It is structured around the concepts of **nodes** and **node trees**. Nodes represent discrete entities of information, encapsulating specific attributes and values. These nodes are organized hierarchically, connected by references in a tree-like structure that forms diffrent types parent-child relationships. This architecture allows for the efficient representation of objects and complex data relationships.

Node
----

A **Node** is a universal class designed to represent one or more memory locations in the system, with the capability to relate these locations hierarchically. The Node class can represent various structures, such as simple variables, lists, or more complex entities like a Genicam tree or an OPC-UA server.

Different sub-classes of nodes are defined to serve specific roles.

ObjectNode
^^^^^^^^^^ 

Represents an organizational structure, typically used to group other nodes.

.. code-block:: cpp

	// Object Node
	ObjectNode* folder_node = ObjecNode("my_folder");

Detailed info of ObjectNode API can be found here here :ref:`objectnode_api`.


DataNode
^^^^^^^^

Contains a value pointer representing a data variable of a certain type (i.e., multidimensional array of BaseType link). There are three types of DataNodes:

- **Memory Managed DataNodes**: Nodes that manage the memory they point to.

- **Not Memory Managed DataNodes**: Nodes that do not manage the memory they point to, further divided into:

	- **Streamed**: The data value is streamed through a Queue’s circular buffer. 
	- **Not Streamed**: The data values that is not streamed through a Queue’s circular buffer.

.. code-block:: cpp
	
	// Different ways of instantiate DataNodes

	// Full constructor (prefered for novel users)
	// - memory managed, 3d double array
	double array[] = {6, 8, 6};
	DataNode *node_0 = new DataNode("my_node_0", EP_64F, {3}, array, true /*memory_managed*/,
									false /*streamed*/, "a 3d vector" /*tooltip*/, W /*accessmode*/);
	node_0->print();

	// (Advanced) Implicit memory_managed DataNode instatiation
	// - memory managed, double, scalar
	DataNode *node_1 = new DataNode("my_node_1", EP_64F, {1});
	node_1->print();

	// (Advanced) Implicit memory_managed DataNode instatiation
	// - memory managed, double, 2x2 array
	DataNode *node_2 = new DataNode("my_node_2", EP_64F, {2, 2});
	node_2->print();

	// (Advanced) Implicit not memory_managed DataNode instatiation
	// - not memory managed, float, scalar
	float data = 40;
	DataNode *node_3 = new DataNode("my_node_3", EP_32F, {1}, &data);
	node_3->print();

	// (Advanced) When instatiateted with a nullptr as address,
	// the DataNode is supposed to be streamed
	// - not memory managed, streamed, int32_t, scalar
	DataNode *node_4 = new DataNode("my_node_4", EP_32S, {1}, nullptr);
	node_4->print();

Detailed info of DataNode API can be found here here :ref:`datanode_api`.

CommandNode
^^^^^^^^^^^

Represents a function that can be invoked without parameters.

.. code-block:: cpp
	
	// Command Node
	CommandNode my_command("reset");

Detailed info of CommandNode API can be found here here :ref:`commandnode_api`.

References
^^^^^^^^^^

To group them and establish its relationships, child nodes need to be dynamically allocated. Once one node is added as child of other node, the parent acquires ownership of it. Therefore, its lifecycle will be linked to its parent. This way, deleting the root node of a tree, also automatically deletes the whole tree. Check the following example: 

.. code-block:: cpp
	
	// Adding references
	my_folder->addReference(epf::EP_HAS_CHILD,node_0);
	my_folder->addReference(epf::EP_HAS_CHILD,node_1);
	my_folder->addReference(epf::EP_HAS_CHILD,node_2);
	my_folder->addReference(epf::EP_HAS_CHILD,node_3);
	node_0->addReference(epf::EP_HAS_CHILD,node_4);
	

	// When deleting the root node every child and their memory (if managed) is destroyed as well.
	delete my_folder;
	

NodeTree
--------

NodeTree class implements methods that make it easier to navigate, process, and control the three of nodes. Two main classes are derived from NodeTree: **Message** which represents the data packets stream through **Queues**, and **Settings** which provides an interface for the control parameters of each filter.

Check additional info of *NodeTree* API here :ref:`nodetree_api`.

Message
-------

Messages are the same-sized information data packets communicated between Filters through Queues. Their organisation is described using *Nodes*, inheriting from *NodeTree*. This way filters can interpret the data. They can contain both static and streaming data (the actual bits passed through the Queue's circular buffer). Messages are at the same time break down in **Items**.

Check additional info of *Message* API here :ref:`message_api`.

Item
^^^^

An item is the basic element of data with ontological sense. In EPF, items are implmented as child nodes of Message root node.

..
  (OPC-UA [REF2](https://reference.opcfoundation.org/Core/Part8/v105/docs/4#Figure1) and [REF2](https://reference.opcfoundation.org/Core/Part8/v105/docs/3.1.1)).




.. code-block:: cpp

	std::string current_units = "mW";
    DataNode current_units = new DataNode(EP_STRING, {1}, &current_units, true);
    
    DataNode current = new DataNode(EP_32S, {1}, nullptr);
    current->addReference(current_units,EP_PROPERTY);

	std::string voltage_units = "mV";
    DataNode voltage_units = new DataNode(EP_STRING, {1}, &voltage_units, true);
    
    DataNode voltage = new DataNode(EP_32S,{1},nullptr);
    voltage->addReference(voltage_units,EP_PROPERTY); // voltage has ownership of voltage units DN memory
   
    Message my_msg;  
    msg.addItem(current); // msg get ownership of current memory
    msg.addItem(voltage); // msg get ownership of voltage memory
    msg.toYaml();         // serialization of message in YAML
   
Output:

.. code-block:: text

    PENDENT

    
Settings
--------

Settings provide an interface for adding modifiable parameters to *Filters*. They are implmented using the *Settings* class and they are embedded in the abstract *Filter* class as a varaible. Internally, they are expressed also as using *Nodes* by inheriting from the base class *NodeTree*. Therefore, each settings binds to a class variable without actually manage its memory.

The *Filter* base class offers a methods to add settings, to modify them or to read/write from files; so the user does not need to manage it directly.


When added as a setting, *Filter* also provides a method that reads a YAML::Node and parse settings values. The files have the following structure:

.. code-block:: yaml
	## config.yaml
	filters:	
	  - name: filter
	    type: MyFilter
	    filter_settings:
          exposure: 500

Here is a *Filter* with a constructor adding filters adding, accessing and modifying settings:

.. code-block:: cpp
      
	#include <embedded-pipelines/core/core.h>
	#include <stdio.h>

	class MyFilter : public epf::Filter
	{
	public:
	  double exposure_time_ = 100.;

	  MyFilter(YAML::Node &config) : Filter()
	  {
		
		// Link "exposure_variable" to "exposure" setting
		this->addSetting("exposure", exposure_time_);

		// Changing the variable also changes the value in setting
		exposure_time_ = 200.;
		std::cout << "exposure: " << *settingValue<double>("exposure") << std::endl; // 200.

		// Using the set method also updates the original value
		this->setSettingValue<double>("exposure", 10.);
		std::cout << "exposure: " << exposure_time_ << std::endl; // 10.

		double new_value = 150.;
		// Copies the value to the original address
		this->setSettingValue("exposure", new_value);
		std::cout << "exposure_time (variable): " << exposure_time_ << std::endl; // 150.

		readSettings(config);
	  };

	protected:
	  int32_t _job() { return 0; };
	  int32_t _open() { return 0; };
	  int32_t _close() { return 0; };
	  int32_t _set() { return 0; };
	  int32_t _reset() { return 0; };
	  int32_t _start() { return 0; };
	  int32_t _stop() { return 0; };
	};

	int main()
	{
	  std::cout << "Here is my filter using settings." << std::endl;
	  std::string fname = "../config.yml";
  	  YAML::Node config = YAML::LoadFile(fname)["filters"][0];
	  MyFilter filter(config);

	  std::cout << "exposure (from main): "<< *filter.settingValue<double>("exposure") << std::endl;

	  return 0;
	}
   
   


Check additional info of *Settings* API here :ref:`settings_api`.

.. References
.. https://codereview.stackexchange.com/questions/11841/c-tree-base-node
