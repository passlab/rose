! { dg-do compile }
! { dg-require-effective-target tls }

      INTEGER FUNCTION INCREMENT_COUNTER()
        COMMON/A22_COMMON/COUNTER
!$OMP THREADPRIVATE(/A22_COMMON/)
        COUNTER = COUNTER +1
        INCREMENT_COUNTER = COUNTER
        RETURN
      END FUNCTION INCREMENT_COUNTER
