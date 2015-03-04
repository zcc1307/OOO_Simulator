#include "ooo_simulator.h"

int main(int argc, char** argv)
{

	const char stage_mapping[10] = {' ','F','D','I','E','C','A','M'};
	int ppl_diag[1000][2000];
	memset(ppl_diag, 0, sizeof(int)*300*300);
	int i, j, num_cycles = 1000;
	

	c_widget widget(argv[1]);

	for (i = 0; i < num_cycles; i++)
	{
		widget.calc();
		widget.edge();	

		for (j = 0; j < widget.instr_table.top; j++)
			ppl_diag[j][i] = widget.instr_table.ins[j].stage;

	}

	for (i = 0; i < widget.instr_table.top;i++)
	{
		//cout<<widget.instr_table.ins[i].pc<<"\t";
		for (j = 0; j < num_cycles; j++)
			if (j == 0 || ppl_diag[i][j] != ppl_diag[i][j-1])
				cout<<stage_mapping[ppl_diag[i][j]]<<" ";
			else
				cout<<"  ";
		cout<<endl;
	}

	
}
