;; Aegen L2 Keysets for Kadena
;; These keysets control governance and operator permissions

;; Admin keyset - controls module upgrades
(define-keyset "free.aegen-admin" (read-keyset "aegen-admin"))

;; Operator keyset - controls batch submissions
(define-keyset "free.aegen-operators" (read-keyset "aegen-operators"))
