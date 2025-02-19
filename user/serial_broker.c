#include "stdio.h"
#include "string.h"
#include "sys/ipc.h"

#include "lib.h"
#include "serial_driver.h"

[[noreturn]] int
main ()
{
  printf ("Hello from serial broker\n");

  while (true)
    {
      word_t badge;
      message_info_t info;

      wait (serial_broker_notification_cap, &badge);

      // there is data to read
      while (!serial_driver_ring_empty (shared_ring))
        {
          info = recv (serial_broker_endpoint_cap, &badge);

          if (get_message_label (info) != serial_driver_read)
            {
              reply (0);
              continue;
            }

          char buffer[100];
          size_t size = serial_driver_ring_read (shared_ring, buffer, 100);

          for (size_t i = 0; i < size; i++)
            set_mr (i, buffer[i]);

          info = new_message_info (0, 0, 0, size);
          reply (info);
        }
    }
}
