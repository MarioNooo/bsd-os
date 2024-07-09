/* Copyright (C) 2000  Free Software Foundation

   This file is part of libgcj.

This software is copyrighted work licensed under the terms of the
Libgcj License.  Please consult the file "LIBGCJ_LICENSE" for
details.  */
 
package javax.naming;

public class OperationNotSupportedException extends NamingException
{
  public OperationNotSupportedException()
  {
    super();
  }

  public OperationNotSupportedException(String msg)
  {
    super(msg);
  }
}