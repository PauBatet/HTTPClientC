#include<views.c>

Route routes[] = {
	{"/", home},
  {"/wait", wait},
  {"/create-user", create_user_view},
  {"/user/<DNI>/profile", create_user_view},
  {"/example", example}
};
