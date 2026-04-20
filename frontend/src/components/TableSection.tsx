import type { ReactNode } from 'react'

type TableSectionProps = {
  title: string
  children: ReactNode
}

export default function TableSection({ title, children }: TableSectionProps) {
  return (
    <section className="table-section">
      <h2 className="section-title">{title}</h2>
      {children}
    </section>
  )
}