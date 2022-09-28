#include<stdio.h>
void main()
{  

	int res,num;

	printf("Enter the number\n");
	scanf("%d\n",&num);

	printf("Which bit you want to clear:\n);

	res = num & (~(0x01 << num));

	printf("After clearing a bit:\n",res);

}
