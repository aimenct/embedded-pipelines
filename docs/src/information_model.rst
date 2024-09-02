Information Model
=================

The information model in the Edge Processing Pipelines Framework (EPPF) is structured around the concepts of **nodes** and **node trees**. Nodes represent discrete entities of information, encapsulating specific attributes and values. These nodes are organized hierarchically, connected by references in a tree-like structure that forms parent-child relationships. This architecture allows for the efficient representation of objects and complex data relationships.

The information model is versatile, supporting various types of data and relationships (reference types). Nodes can be used to represent everything from simple variables to more complex objects like a Genicam camera description file, Asset Administration Shell model, or an OPC-UA server. This flexibility makes the information model integral to managing and interacting with the data in the application.

Node
----

A **Node** is a universal class designed to represent one or more memory locations in the system, with the capability to relate these locations hierarchically. The Node class can represent various structures, such as simple variables, lists, or more complex entities like a Genicam tree or an OPC-UA server.

Different classes of nodes are defined to serve specific roles:

- **ObjectNode**: Represents an organizational structure, typically used to group other nodes.
- **DataNode**: Contains a value pointer representing a data variable of a certain type (i.e., multidimensional array of BaseType link). There are three types of DataNodes:

  - **Memory Managed DataNodes**: Nodes that manage the memory they point to.

  - **Not Memory Managed DataNodes**: Nodes that do not manage the memory they point to, further divided into:

    - **Streamed**: The data value is streamed through a Queue’s circular buffer. 
    - **Not Streamed**: The data values that is not streamed through a Queue’s circular buffer.

- **CommandNode**: Represents a function that can be invoked without parameters.

.. code-block:: cpp

	# Object Node
	ObjectNode my_folder("my folder 1");

	# Instantiation of different DataNodes
	# - memory managed, double, scalar 
	DataNode node_1("my node 1", EP_64F, {1});
	node_1->print()

	# - memory managed, double, 2x2 array
	DataNode node_2("my node 2", EP_64F, {2,2});
	node_2->print()

	float data = 40;
	# - not memory managed, float, scalar
	DataNode node_3("my node 3", EP_32S, {1}, &data);
	node_3->print()

	# - streamed, float, scalar
	DataNode node_4("my node 4", EP_32S, {1}, nullptr);
	node_4->print()

	# Command Node
	CommandNode my_command("reset");

	# NodeTree
	my_folder.addReference(ep::EP_HAS_CHILD,node_1);
	my_folder.addReference(ep::EP_HAS_CHILD,node_1);
	my_folder.addReference(ep::EP_HAS_CHILD,node_1);
	my_folder.addReference(ep::EP_HAS_CHILD,node_1);

	my_folder.printTree();

Output:
	
.. code-block:: text

   Output from my_node->print():
   [Your output here, e.g., details of the DataNode structure]

   Output from memory_managed_node->print():
   [Your output here, e.g., details of the DataNode structure]

   Output from streamed_node->print():
   [Your output here, e.g., details of the DataNode structure] 

- **NodeTree** class is provided to efficiently manage the hierarchically node-tree structure. It implements methods that make it easier to navigate, process, and control the three of nodes. Two main classes are derived from NodeTree: **Message** which represents the data packets stream through **Queues**, and **Settings** which provides an interface for the control parameters of each filter.

Message
-------

Messages are the unit of information communicated between Filters that are placed in the message Queues.
Messages are structured in a node tree-based format following the information model described in section XX to ensure filters can correct interpret the data.
It contains static data and streaming data (data passed through the Queue's circular buffer).
To facilitate the serach of data within a Messages, these are at the same time break down in Items.

```cpp
class Messages
```

Item
^^^^

- **Items**: Is the basic element of data with ontological sense. In the frame of a Message, an Item is the root node childs (OPC-UA [REF2](https://reference.opcfoundation.org/Core/Part8/v105/docs/4#Figure1) and [REF2](https://reference.opcfoundation.org/Core/Part8/v105/docs/3.1.1)).

```cpp
Item
```

.. code-block:: yaml
		
	name: RGB
	type: ArvCam
	filter_settings:
          timeout: 10
	  counter init: 0
	  metadata: This is a dummy Filter.
	device_settings:
	  dummy_setting: 0
	  dummy_offset: 50	
	queue_settings:
	  sources: 1
	  sinks: 1
	  sink queues:
	  - id: 0
	    length: 10
	    type: lifo
	    max consumers: 5
	  writers:
	  - id: 0
	    batch: 10
	    blocking: true
	  readers:
	  - id: 0
	    batch: 10
	    new per batch: 5
	    blocking: true


Messages are the data packets that pass through the pipeline. Message class serves two main purposes: i) define the equally sized data schema of the Messages that will be pass through a **Queue**, ii) 

Messages are node trees with one root node. To facilitate the navigation messages are organized in **Items**. Items are the childs of the root node, can be simple DataNodes or more complex Object structures.

.. code-block:: cpp

    DataNode current_units = new DataNode(EP_STRING,{1});
    sprintf(current_units->value(),"mW");
    
    DataNode current = new DataNode(EP_32S,{1},nullptr);
    current->hasChild(current_units,EP_PROPERTY);

    DataNode voltage_units = new DataNode(EP_STRING,{1});
    sprintf(voltage_units->value(),"mV");
    
    DataNode voltage = new DataNode(EP_32S,{1},nullptr);
    voltage->hasChild(voltage_units,EP_PROPERTY); // voltage has ownership of voltage units DN memory
   
    Message my_msg;  
    msg.addItem(current); // msg get ownership of current memory
    msg.addItem(voltage); // msg get ownership of voltage memory
    msg.toYaml();         // serialization of message in YAML
   
Output:

.. code-block:: text

    Tree like strcuture message

    
Settings
--------

Settings configure the behavior of Filters and Pipelines.

.. code-block:: cpp
      
   Settingse my_settings;
   my_settings.addSetting();


References
https://codereview.stackexchange.com/questions/11841/c-tree-base-node
