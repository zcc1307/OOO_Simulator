//Out of Order Simulator
//A53021318 Chicheng Zhang
//Date:12.19.2014
#include <iostream>
#include <fstream>
#include <queue>
#include <list>
#include <cstring>
#include <iomanip>

#define FETCH 1
#define DECODE 2
#define ISSUE 3
#define EXECUTE 4
#define COMMIT 5
#define EA 6
#define MEMORY 7


using namespace std;


int min(int x, int y)
{
	return x < y ? x : y;
}

class c_widget;

class c_trace_line
{
public:
	c_trace_line();
	c_trace_line(int n_pc, char n_oper, int n_rs, int n_rt, int n_rd);
	c_trace_line(int n_pc, char n_oper, int n_rs, int n_rt, int n_rd, int n_mem_addr);
	c_trace_line(int n_pc, char n_oper, int n_rs, int n_rt, int n_rd, bool n_branch_bit);
	char oper;
	int rs, rt, rd;
	int mem_addr;	
	bool branch_bit;
	int pc;
};

c_trace_line::c_trace_line()
{

}

c_trace_line::c_trace_line(int n_pc, char n_oper, int n_rs, int n_rt, int n_rd)
{
	pc = n_pc;
	oper = n_oper;
	rs = n_rs;
	rt = n_rt;
	rd = n_rd;
}

c_trace_line::c_trace_line(int n_pc, char n_oper, int n_rs, int n_rt, int n_rd, int n_mem_addr)
{
	pc = n_pc;
	oper = n_oper;
	rs = n_rs;
	rt = n_rt;
	rd = n_rd;
	mem_addr = n_mem_addr;
}

c_trace_line::c_trace_line(int n_pc, char n_oper, int n_rs, int n_rt, int n_rd, bool n_branch_bit)
{
	pc = n_pc;
	oper = n_oper;
	rs = n_rs;
	rt = n_rt;
	rd = n_rd;
	branch_bit = n_branch_bit;
}


class c_instruction: public c_trace_line
{
public:
	c_instruction();
	c_instruction(c_trace_line);
	char stage; // which stage is the instruction in
	int rps, rpt, rpd; // physical register allocated
	bool ready; // are all operands of the instruction ready?
	int addr_status; //if it is an address instruction, what status is it in??
	bool dest_allocated;
	int old_phys_reg;
	bool done;
};

c_instruction::c_instruction(){}

c_instruction::c_instruction(c_trace_line ctl): c_trace_line(ctl)
{
	stage = 0;
	rps = 0;
	rpt = 0;
	rpd = 0;
	ready = false;
	addr_status = 0;
	dest_allocated = false;
	old_phys_reg = -1;
	done = false;
}

class c_module
{
public:
	c_widget* parent;
	virtual int calc();
	virtual int edge();
};

int c_module::calc(){}
int c_module::edge(){}


class c_instr_memory: public c_module
{
public: 
	virtual int calc();
	virtual int edge();
	c_widget* parent;
	int init(char*, int, c_widget*);	
	int size;
	int pc;
	int fetch_width;
	c_trace_line trace_line_buffer[1000];
};

int c_instr_memory::init(char* filename, int n_fetch_width, c_widget* p_parent)
{
	fetch_width = n_fetch_width;
	parent = p_parent;
	ifstream fin(filename);

	if (!fin)
		cout<<"file open error!"<<endl;

	char oper;
	int rs, rt, rd, mem_addr, branch_number;
	int i = 0;
	while (fin>>oper)
	{
		if (oper == 'M' || oper == 'A' || oper == 'I')
		{
			fin>>hex>>rs>>rt>>rd;
			trace_line_buffer[i] = c_trace_line(i, oper, rs, rt, rd);
		}
		else if (oper == 'L' || oper == 'S')
		{
			fin>>hex>>rs>>rt>>rd>>mem_addr;
			trace_line_buffer[i] = c_trace_line(i, oper, rs, rt, rd, mem_addr);
		}
		else if (oper == 'B')
		{
			fin>>hex>>rs>>rt>>rd>>branch_number;
			trace_line_buffer[i] = c_trace_line(i, oper, rs, rt, rd, (branch_number == 1));
		}
		i++;
	}
	size = i;
	pc = 0;
	fin.close();

}

class c_instr_table
{
public:
	c_widget* parent; 
	int init(c_widget*);
	int top;
	c_instruction ins[1000];
};

int c_instr_table::init(c_widget* p_parent)
{
	parent = p_parent;
}

class c_instr_cache: public c_module
{
public:
	c_widget* parent;
	int init(int,c_widget*);
	virtual int calc();
	virtual int edge();
	int size;
	list<c_instruction*> instr_queue; 
};

int c_instr_cache::init(int n_size, c_widget* p_parent)
{
	parent = p_parent;
	size = n_size;
}

class c_decode_branch: public c_module
{
public:
	c_widget* parent;
	int init(int,int,int,c_widget*);
	virtual int calc();
	virtual int edge();
	int size;
	int issue_bandwidth;
	int translation_bandwidth;
	list<c_instruction*> db_queue;
};

int c_decode_branch::init(int n_size, int n_issue_bandwidth, int n_translation_bandwidth, c_widget* p_parent)
{
	issue_bandwidth = n_issue_bandwidth;
	translation_bandwidth = n_translation_bandwidth;
	parent = p_parent;
	size = n_size;
}

class c_reg_map_table: public c_module
{
public:
	c_widget* parent;
	int init(int, c_widget*);
	int virtual_size;
	int v_to_p[1000];
	//virtual int calc();
	//virtual int edge();
	int update_map_table(int, int);
	int lookup_map_table(int);
};

int c_reg_map_table::init(int n_virtual_size, c_widget* p_parent)
{
	parent = p_parent;
	virtual_size = n_virtual_size;
	//initial mapping
	for (int i = 0; i < virtual_size; i++)
	{
		v_to_p[i] = i;
	}
}

int c_reg_map_table::update_map_table(int v, int p)
{
	v_to_p[v] = p;
}

int c_reg_map_table::lookup_map_table(int v)
{
	return v_to_p[v];
}



class c_free_list: public c_module
{
public:
	c_widget* parent;
	int init(int,int,c_widget*);
	//virtual int calc();
	//virtual int edge();
	int get_free();
	int set_not_free(int);
	int set_free(int);
	int physical_size;
	bool free[1000];
};

int c_free_list::init(int n_physical_size, int n_virtual_size, c_widget* p_parent)
{
	parent = p_parent;
	physical_size = n_physical_size;
	//initialization: 1-virtual allocated
	for (int i = 0; i < physical_size; i++)
	{
		free[i] = true;		
	}
	// to be consistent with map table
	for (int i = 0; i < n_virtual_size; i++)
	{
		free[i] = false;
	}
};

int c_free_list::get_free()
{
	for (int i = 0; i < physical_size; i++)
	{
		if (free[i])
			return i;
	}
	return -1;
}

int c_free_list::set_not_free(int i)
{
	free[i] = false;
}

int c_free_list::set_free(int i)
{
	free[i] = true;
}


class c_busy_bit_table: public c_module
{
public: 
	c_widget* parent;
	int init(int,c_widget*);
	//virtual int calc();
	//virtual int edge();
	bool is_busy(int);
	int set_busy(int);
	int set_not_busy(int);
	int physical_size;
	bool busy[1000];
};

int c_busy_bit_table::init(int n_physical_size, c_widget* p_parent)
{
	parent = p_parent;
	physical_size = n_physical_size;
	for (int i = 0; i < physical_size; i++)
	{
		busy[i] = false;
	}
}

bool c_busy_bit_table::is_busy(int i)
{
	return busy[i];
}

int c_busy_bit_table::set_busy(int i)
{
	busy[i] = true;
}

int c_busy_bit_table::set_not_busy(int i)
{
	busy[i] = false;
}

class c_elem_branch_stack
{
public:
	c_elem_branch_stack(c_reg_map_table, c_free_list, c_instruction*);
	c_reg_map_table rmt;
	c_free_list fl;
	c_instruction* ins;
};

c_elem_branch_stack::c_elem_branch_stack(c_reg_map_table n_table, c_free_list n_list, c_instruction* n_ins)
{
	rmt = n_table;
	fl = n_list;
	ins = n_ins;
}

class c_branch_stack: public c_module
{
public: 
	c_widget* parent;
	int init(int,c_widget*);
	int height();
	int stack_size;
	list<c_elem_branch_stack> st;
};

int c_branch_stack::init(int n_stack_size, c_widget* p_parent)
{
	parent = p_parent;
	stack_size = n_stack_size;
}

int c_branch_stack::height()
{
	return st.size();
}


/*class c_elem_active_list
{
	c_instruction* ins;
	bool done;
	int old_reg;	
};*/

class c_active_list: public c_module
{
public:
	c_widget* parent;
	int init(int,int,c_widget*);
	virtual int calc();
	virtual int edge();
	int size;
	int grad_bandwidth;
	list<c_instruction*> al;
};

int c_active_list::init(int n_size, int n_grad_bandwidth, c_widget* p_parent)
{
	grad_bandwidth = n_grad_bandwidth;
	parent = p_parent;
	size = n_size;
}

class c_fp_queue: public c_module
{
public:
	c_widget* parent;
	int init(int,c_widget*);
	virtual int calc();
	virtual int edge();
	int size;
	list<c_instruction*> fpq;
};

int c_fp_queue::init(int n_size, c_widget* p_parent)
{
	parent = p_parent;
	size = n_size;
}

class c_fp_unit: public c_module
{
public:
	c_widget* parent;
	int init(c_widget*);
	virtual int calc();
	virtual int edge();
	c_instruction* ins[3];
};

int c_fp_unit::init(c_widget* p_parent)
{
	parent = p_parent;
	for(int i = 0; i < 3; i++)
		ins[i] = NULL;
}

class c_int_queue: public c_module
{
public:
	c_widget* parent;
	int init(int,c_widget*);
	virtual int calc();
	virtual int edge();
	int size;
	list<c_instruction*> inq;
};

int c_int_queue::init(int n_size, c_widget* p_parent)
{
	parent = p_parent;
	size = n_size;
}

class c_int_unit: public c_module
{
public: 
	c_widget* parent;
	int init(c_widget*);
	virtual int calc();
	virtual int edge();
	c_instruction* ins;
};

int c_int_unit::init(c_widget* p_parent)
{
	parent = p_parent;
	ins = NULL;
}

/*class c_elem_addr_queue
{
public:

	c_instruction* ins;
	int status;
};*/

class c_addr_queue: public c_module
{
public: 
	c_widget* parent;
	int init(int,c_widget*);
	virtual int calc();
	virtual int edge();
	int size;
	list<c_instruction*> adq;
	//queue<c_elem_addr_queue> adq;
};

int c_addr_queue::init(int n_size, c_widget* p_parent)
{
	parent = p_parent;
	size = n_size;
}

class c_mem_unit: public c_module
{
public:
	c_widget* parent;
	int init(c_widget*);
	virtual int calc();
	virtual int edge();	
	c_instruction* ins;
};

int c_mem_unit::init(c_widget* p_parent)
{
	parent = p_parent;
	ins = NULL;
}

class c_addr_unit: public c_module
{
public:
	c_widget* parent;
	int init(c_widget*);
	virtual int calc();
	virtual int edge();	
	c_instruction* ins;
};

int c_addr_unit::init(c_widget* p_parent)
{
	parent = p_parent;
	ins = NULL;
}


class c_reg_file: public c_module
{
public:
	c_widget* parent;
	int init(c_widget*);
	//virtual int calc();
	//virtual int edge();

};

int c_reg_file::init(c_widget* p_parent)
{
	parent = p_parent;
}


class c_widget: public c_module
{
public:
	c_widget(char*);
	virtual int calc();
	virtual int edge();
	c_instr_memory instr_memory;
	c_instr_table instr_table;
	c_instr_cache instr_cache;
	c_decode_branch decode_branch;
	c_reg_map_table reg_map_table;
	c_free_list free_list;
	c_busy_bit_table busy_bit_table;
	c_branch_stack branch_stack;
	c_active_list active_list;
	c_fp_queue fp_queue;	
	c_fp_unit fp_mul_unit, fp_add_unit;
	c_int_queue int_queue;
	c_int_unit int_unit_1, int_unit_2;
	c_addr_queue addr_queue;
	c_addr_unit addr_unit;
	c_mem_unit mem_unit;
	c_reg_file reg_file;	
};

c_widget::c_widget(char* str)
{
	int physical_size = 64, virtual_size = 32;
	int scalar = 1;
	int max_commit = 4;
	
	instr_memory.init(str,scalar, this);
	instr_table.init(this);
	instr_cache.init(4,this);
	decode_branch.init(8,scalar,scalar,this);
	reg_map_table.init(virtual_size, this);
	free_list.init(physical_size, virtual_size, this);
	busy_bit_table.init(physical_size, this);
	branch_stack.init(4, this);
	active_list.init(32,max_commit,this);
	fp_queue.init(16, this);
	fp_mul_unit.init(this);
	fp_add_unit.init(this);
	int_queue.init(16, this);
	int_unit_1.init(this);
	int_unit_2.init(this);
	addr_queue.init(16,this);
	addr_unit.init(this);
	mem_unit.init(this);
	reg_file.init(this);

}


int c_widget::edge()
{

	// the order might be totally wrong
	reg_file.edge(); //issuing outputs to its descendants
	fp_add_unit.edge();
	fp_mul_unit.edge();
	int_unit_1.edge();
	int_unit_2.edge();
	mem_unit.edge();
	addr_unit.edge();

	fp_queue.edge();
	int_queue.edge();
	addr_queue.edge();

	active_list.edge();

	free_list.edge();
	reg_map_table.edge();
	decode_branch.edge();

	busy_bit_table.edge();

	instr_cache.edge();
	instr_memory.edge();

}

int c_widget::calc()
{
	reg_file.calc(); //issuing outputs to its descendants
	fp_add_unit.calc();
	fp_mul_unit.calc();
	int_unit_1.calc();
	int_unit_2.calc();
	fp_queue.calc();
	int_queue.calc();
	mem_unit.calc();
	addr_unit.calc();
	addr_queue.calc();
	active_list.calc();

	free_list.calc();
	reg_map_table.calc();
	decode_branch.calc();

	busy_bit_table.calc();

	instr_cache.calc();
	instr_memory.calc();

}

int c_instr_memory::calc()
{

}

int c_instr_memory::edge()
{
	int remaining_size = parent->instr_cache.size - parent->instr_cache.instr_queue.size();
	remaining_size = min(size-pc, remaining_size);
	remaining_size = min(fetch_width, remaining_size);
	for (int i = 0; i < remaining_size; i++)
	{
		parent->instr_table.ins[parent->instr_table.top] = c_instruction(trace_line_buffer[pc]);
		c_instruction* ins = &(parent->instr_table.ins[parent->instr_table.top]);
		ins->stage = FETCH;
		parent->instr_cache.instr_queue.push_back(ins);
		parent->instr_table.top++;
		pc++;
	}
		
}

int c_instr_cache::calc()
{}

int c_instr_cache::edge()
{
	int remaining_size = parent->decode_branch.size - parent->decode_branch.db_queue.size();
	remaining_size = min(remaining_size, instr_queue.size());	
	for (int i = 0; i < remaining_size; i++)
	{
		c_instruction* ins = instr_queue.front();
		ins->stage = DECODE;
		parent->decode_branch.db_queue.push_back(ins);
		instr_queue.pop_front();
	}

}

int c_decode_branch::calc()
{
	int count = 0;

	//virtual -> physical translation
	list<c_instruction*>::iterator it;
	for (it = db_queue.begin(); it != db_queue.end() && count < translation_bandwidth; it++)
	{

		// Then find a instruction that needs virtual -> physical translation
		if ((*it)->dest_allocated)
			continue;

		if ((*it)->oper == 'S' || (*it)->oper == 'B')
		{
			// do the translation of source
			(*it)->rps = parent->reg_map_table.lookup_map_table((*it)->rs);
			(*it)->rpt = parent->reg_map_table.lookup_map_table((*it)->rt);

			(*it)->dest_allocated = true;
			count++;
		}

		else
		{	
			// First find a slot
			int slot = parent->free_list.get_free();
			if (slot == -1)
			{
				break;			
			}
			
			if ((*it)->oper == 'I' || (*it)->oper == 'A' || (*it)->oper == 'M')
			{
				// do the translation of source
				(*it)->rps = parent->reg_map_table.lookup_map_table((*it)->rs);
				(*it)->rpt = parent->reg_map_table.lookup_map_table((*it)->rt);

				// do the translation of dest
				parent->free_list.set_not_free(slot);
				(*it)->old_phys_reg = (parent->reg_map_table).lookup_map_table((*it)->rd);
				(parent->reg_map_table).update_map_table((*it)->rd, slot);
				(*it)->rpd = slot;
				parent->busy_bit_table.set_busy(slot);

				(*it)->dest_allocated = true;
				count++;
			}
			else if ((*it)->oper == 'L')
			{
				// do the translation of source
				(*it)->rps = parent->reg_map_table.lookup_map_table((*it)->rs);

				// do the translation of dest
				parent->free_list.set_not_free(slot);
				(*it)->old_phys_reg = (parent->reg_map_table).lookup_map_table((*it)->rt);
				(parent->reg_map_table).update_map_table((*it)->rt, slot);
				(*it)->rpt = slot;
				parent->busy_bit_table.set_busy(slot);

				(*it)->dest_allocated = true;
				count++;
			}	
		}
	}
}

int c_decode_branch::edge()
{
	//issue to active list and queues

	for (int i = 0; i < issue_bandwidth; i++)
	{
		c_instruction* ins = db_queue.front();

		// haven't done the virtual to physical translation: cannot issue
		if (!ins->dest_allocated)
			break;

		// branch, and height is large: cannot issue
		if (ins->oper == 'B' && parent->branch_stack.height() >= parent->branch_stack.stack_size)
			break;

		// cannot find an empty slot on active list
		else if (parent->active_list.size == parent->active_list.al.size())
			break;
		else
		{
			if (ins->oper == 'A' || ins->oper == 'M')
			{
				if (parent->fp_queue.size == parent->fp_queue.fpq.size())
					break;
				else
				{
					ins->stage = ISSUE;	
					parent->fp_queue.fpq.push_back(ins);
					parent->active_list.al.push_back(ins);
					db_queue.pop_front();
				}

			}

			else if (ins->oper == 'I' || ins->oper == 'B')
			{
				if (parent->int_queue.size == parent->int_queue.inq.size())
					break;
				else
				{
					ins->stage = ISSUE;	
					parent->int_queue.inq.push_back(ins);
					parent->active_list.al.push_back(ins);
					db_queue.pop_front();

					// store the current Register Map Table and Free List into the branch stack
					if (ins->oper == 'B')
					{
						parent->branch_stack.st.push_back(c_elem_branch_stack(parent->reg_map_table, parent->free_list, ins));		
					}

				}
			}
		
			else if (ins->oper == 'L' || ins->oper == 'S')
			{
				if (parent->addr_queue.size == parent->addr_queue.adq.size())
					break;
				else
				{
					ins->addr_status = 0;
					ins->stage = ISSUE;	
					parent->addr_queue.adq.push_back(ins);
					parent->active_list.al.push_back(ins);
					db_queue.pop_front();
				}
			}
		
		}
			
	}
}

int c_active_list::calc()
{}

int c_active_list::edge()
{
	for (int i = 0; i < grad_bandwidth; i++)
	{
		if (!al.empty())		
		{
			c_instruction* ins = al.front();

			if (ins->done)
			{
				if (ins->oper == 'I' || ins->oper == 'A' || ins->oper == 'M' || ins->oper == 'L')
					parent->free_list.set_free(ins->old_phys_reg);
				ins->stage = COMMIT;

				//load and store have to do cleanup here since they do not come outside the queue yet
				if (ins->oper == 'L' || ins->oper == 'S')
					parent->addr_queue.adq.pop_front();

				al.pop_front();			
			}
		}
	
	}

}

int c_fp_queue::calc()
{

}

int c_fp_queue::edge()
{
	//set ready bit
	list<c_instruction*>::iterator it;
	for (it = fpq.begin(); it != fpq.end(); it++)
	{
		if (!parent->busy_bit_table.is_busy((*it)->rps) && !parent->busy_bit_table.is_busy((*it)->rpt) )
			(*it)-> ready = true;

	}

	// FP Add
	for (it = fpq.begin(); it != fpq.end(); it++)
	{
		if ((*it)->oper == 'A' && ((*it)->ready))
		{	
			parent->fp_add_unit.ins[0] = *it;
			(*it)->stage = EXECUTE;
			fpq.erase(it);
			break;
		}
	}

	// FP Multiply
	for (it = fpq.begin(); it != fpq.end(); it++)
	{
		if ((*it)->oper == 'M' && ((*it)->ready))
		{
			parent->fp_mul_unit.ins[0] = *it;
			(*it)->stage = EXECUTE;
			fpq.erase(it);
			break;
		}
	}
}

int c_fp_unit::calc()
{

}

int c_fp_unit::edge()
{
	if (ins[2] != NULL)
		ins[2]->done = true;

	//forwarding mechanism
	if (ins[1] != NULL)
		parent->busy_bit_table.set_not_busy(ins[1]->rpd);
		
	
	ins[2] = ins[1];
	ins[1] = ins[0];
	ins[0] = NULL;

}

int c_int_queue::calc()
{

}

int c_int_queue::edge()
{

	//set ready bit
	list<c_instruction*>::iterator it;
	for (it = inq.begin(); it != inq.end(); it++)
	{
		if (!parent->busy_bit_table.is_busy((*it)->rps) && !parent->busy_bit_table.is_busy((*it)->rpt) )
			(*it)-> ready = true;

	}

	bool has_issued_a_branch;

	// Branch
	for (it = inq.begin(); it != inq.end(); it++)
	{
		if ((*it)->oper == 'B' && ((*it)->ready))
		{	
			parent->int_unit_1.ins = *it;
			(*it)->stage = EXECUTE;
			inq.erase(it);
			has_issued_a_branch = true;
			break;
		}
	}

	//Integer
	/*int int_bandwidth;
	if (has_issued_a_branch)
		int_bandwidth = 1;
	else
		int_bandwidth = 2;*/

	//int count = 0;

	for (it = inq.begin(); it != inq.end(); it++)
	{	
		if ((*it)->oper == 'I' && ((*it)->ready))
		{
			parent->int_unit_2.ins = *it;
			(*it)->stage = EXECUTE;
			inq.erase(it);
			break;
		}
	}

	if (!has_issued_a_branch)
	{
		for (it = inq.begin(); it != inq.end(); it++)
		{
			if ((*it)->oper == 'I' && ((*it)->ready))
			{
				parent->int_unit_1.ins = *it;
				(*it)->stage = EXECUTE;
				inq.erase(it);
				break;
			}
		}
	}
}

int c_int_unit::calc()
{

}

int c_int_unit::edge()
{
	if (ins == NULL)
		return 0;

	//branch is mispredicted, undo all the instructions after this branch
	if (ins->oper == 'B' && ins->branch_bit == true)
	{
		//reset pc and modify the trace file
		parent->instr_memory.trace_line_buffer[ins->pc].branch_bit = false;
		parent->instr_memory.pc = ins->pc;

		list<c_instruction*>::iterator it;

		//Clean FP Queue:

		if (parent->fp_queue.fpq.empty() == false)
		{
			list<c_instruction*> temp;
			for (it = parent->fp_queue.fpq.begin(); it != parent->fp_queue.fpq.end(); it++)
			{
				if ((*it)->pc <= ins->pc)
					temp.push_back(*it);
			}
			parent->fp_queue.fpq = temp;
		}

		//Clean Int Queue:
		if (parent->int_queue.inq.empty() == false)
		{
			list<c_instruction*> temp;
			for (it = parent->int_queue.inq.begin(); it != parent->int_queue.inq.end(); it++)
			{
				if ((*it)->pc <= ins->pc)	
					temp.push_back(*it);
			}
			parent->int_queue.inq = temp;
		}
		
		//Clean Addr Queue:
		if (parent->addr_queue.adq.empty() == false)
		{
			list<c_instruction*> temp;
			for (it = parent->addr_queue.adq.begin(); it != parent->addr_queue.adq.end(); it++)
			{
				if ((*it)->pc < ins->pc)
					temp.push_back(*it);
			}
			parent->addr_queue.adq = temp;
		}

		//Clean Decode Branch:

		if (parent->decode_branch.db_queue.empty() == false)
		{
			list<c_instruction*> temp;
			for (it = parent->decode_branch.db_queue.begin(); it != parent->decode_branch.db_queue.end(); it++)
			{
				if ((*it)->pc <= ins->pc)
					temp.push_back(*it);
			}
			parent->decode_branch.db_queue = temp;	
		}
	
		//Clean Instruction cache:

		if (parent->instr_cache.instr_queue.empty() == false)
		{
			list<c_instruction*> temp;
			for (it = parent->instr_cache.instr_queue.begin(); it != parent->instr_cache.instr_queue.end(); it++)
			{
				if ((*it)->pc <= ins->pc)
					temp.push_back(*it);
			}	
			parent->instr_cache.instr_queue = temp;
		}
		

		//Clean Active List:

		if (parent->active_list.al.empty() == false)
		{
			list<c_instruction*> temp;
			for (it = parent->active_list.al.begin(); it != parent->active_list.al.end(); it++)
			{
				if ((*it)->pc <= ins->pc)
					temp.push_back(*it);
			}
			parent->active_list.al = temp;
		}

		//Recover Register Mapping Table and Free List:

		list<c_elem_branch_stack>::iterator iter;
		for (iter = parent->branch_stack.st.begin(); iter != parent->branch_stack.st.end(); iter++)
		{
			c_elem_branch_stack ebs = *iter;
			if (ebs.ins == ins)
			{
				parent->reg_map_table = ebs.rmt;
				parent->free_list = ebs.fl;
				break;
			}

		}



	}


	//after resolving the branch, clean up the branch stack
	list<c_elem_branch_stack>::iterator iter;
		for (iter = parent->branch_stack.st.begin(); iter != parent->branch_stack.st.end(); iter++)
			if ((*iter).ins == ins)
			{	
				parent->branch_stack.st.erase(iter);
				break;
			}
		

	ins->done = true;
	if (ins->oper == 'I')
		parent->busy_bit_table.set_not_busy(ins->rpd);
	ins = NULL;
}

int c_addr_queue::calc()
{
	list<c_instruction*>::iterator it, it2;
	for (it = adq.begin(); it != adq.end(); it++)
	{
		for (it2 = it; it2 != adq.end(); it2++)
		{
			if ((*it)->oper == 'S' && (*it2)->oper == 'L' && (*it)->addr_status == 2 && (*it2)->addr_status == 2 && (*it)->mem_addr == (*it2)->mem_addr)
			{	
				(*it2)->addr_status = 3;
			}

		}
		
	}

}

int c_addr_queue::edge()
{
	list<c_instruction*>::iterator it, it2;
	for (it = adq.begin(); it != adq.end(); it++)
	{
		if ((*it)->addr_status == 0)
		{
			if ((*it)->oper == 'L')
			{
				if (!parent->busy_bit_table.is_busy((*it)->rps))
				{	
					(*it)->ready = true;
					(*it)->addr_status = 1;
				}

			}
			else if ((*it)->oper == 'S')
			{
				if (!parent->busy_bit_table.is_busy((*it)->rps) && !parent->busy_bit_table.is_busy((*it)->rpt))
				{	
					(*it)->ready = true;
					(*it)->addr_status = 1;
				}

			}
		}
	}

	for (it = adq.begin(); it != adq.end(); it++)
	{
		if ((*it)->addr_status == 1)
		{
			(*it)->stage = EA;
			parent->addr_unit.ins = *it;
			break;
		}
	}


	for (it = adq.begin(); it != adq.end(); it++)
	{	
		//Do not attempt to do memory operation after an unresolved address instruction in the queue.
		if ((*it)->addr_status < 2)
			break;	

		if ((*it)->addr_status == 2)
		{
			(*it)->stage = MEMORY;
			parent->mem_unit.ins = *it;
			break;
		}

	}

	//allowed to commit
	if ((*adq.begin())->addr_status == 3)
	{
		(*adq.begin())->done = true;
	}



}

int c_mem_unit::calc()
{
}

int c_mem_unit::edge()
{
	if (ins == NULL)
		return 0;

	ins->addr_status = 3;
	ins->done = true;

	if (ins->oper == 'L')
		parent->busy_bit_table.set_not_busy(ins->rpt);

	ins = NULL;
}

int c_addr_unit::calc()
{

}

int c_addr_unit::edge()
{
	if (ins == NULL)
		return 0;

	ins->addr_status = 2;
	ins = NULL;
}

