set breakpoint pending on
set confirm off
file ./holeycc
break holeyc::Err::report
commands
	where
end
break holeyc::InternalError::InternalError
commands
	where
end
break holeyc::ToDoError::ToDoError
commands
	where
end

define p4
  set args p4_tests/$arg0.holeyc -n --
  run
end
