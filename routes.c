#include<views.c>

Route routes[] = {
	{"/", home},
  {"/create-user", create_user_view},
  {"/user/<DNI>/profile", create_user_view},
  {"/example", example}
};
